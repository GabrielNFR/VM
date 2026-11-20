#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128
#define TLB_SIZE 16
#define INVALID_FRAME (-1)

typedef enum {
    ENDERECO_OK = 0,
    ENDERECO_SEM_DIGITOS,
    ENDERECO_OVERFLOW,
    ENDERECO_LIXO,
    ENDERECO_FORA_DA_FAIXA
} StatusEndereco;

typedef enum {
    POLITICA_FIFO,
    POLITICA_LRU
} PoliticaPagina;

typedef enum {
    TLB_FIFO,
    TLB_LRU
} PoliticaTlb;

typedef struct {
    int pagina;
    int frame;
    int valido;
    unsigned long ultimoUso;
} EntradaTlb;

typedef struct {
    unsigned char frames[NUM_FRAMES][PAGE_SIZE];
    int tabelaPaginas[NUM_PAGES];
    int proximoFrame;
    FILE *backingStore;

    int filaFifo[NUM_FRAMES];
    int inicioFila;
    int fimFila;
    int tamanhoFila;

    PoliticaPagina politicaPagina;
    PoliticaTlb politicaTlb;
    unsigned long ultimoUso[NUM_PAGES];
    unsigned long relogio;

    EntradaTlb tlb[TLB_SIZE];
    int tlbHits;
    int proximaEntradaTlb;
    unsigned long relogioTlb;
    int pageFaults;
} Memoria;

typedef struct {
    int *itens;
    size_t quantidade;
    size_t capacidade;
} AddressList;

typedef struct {
    Memoria *memoria;
    int pagina;
    int indice;
    int encontrou;
} ThreadTlb;

static void *procuraNaTlb(void *arg);
static int escolheEntradaTlbLru(Memoria *memoria);
static int insereTlb(Memoria *memoria, int pagina, int frame);
static int buscaTlb(Memoria *memoria, int pagina);
static int escolheVitimaLru(Memoria *memoria);
static void registraUso(Memoria *memoria, int pagina);
static int escolheVitimaPagina(Memoria *memoria);
static void enfileiraFifo(Memoria *memoria, int pagina);
static int desenfileiraFifo(Memoria *memoria);
static int executaSimulacao(Memoria *memoria, const AddressList *enderecos, FILE *saida);
static StatusEndereco validaEndereco(const char *s, int *saida);
static void liberaLista(AddressList *lista);
static int carregaEnderecos(const char *caminho, AddressList *lista);
static int adicionarLista(AddressList *lista, int valor);
static int inicializaMemoria(Memoria *memoria, PoliticaPagina politicaPagina, PoliticaTlb politicaTlb);
static void liberaMemoria(Memoria *memoria);
static int carregaPagina(Memoria *memoria, int pagina, int frame);
static int traduzEndereco(Memoria *memoria, int enderecoVirtual, int *enderecoFisico, signed char *valor, int *indiceTlb);

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Erro: uso: %s <arquivo> <fifo|lru> <fifo|lru>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[2], "fifo") != 0 && strcmp(argv[2], "lru") != 0) {
        fprintf(stderr, "Erro: política de página inválida: '%s'. Use fifo ou lru.\n", argv[2]);
        return 1;
    }
    if (strcmp(argv[3], "fifo") != 0 && strcmp(argv[3], "lru") != 0) {
        fprintf(stderr, "Erro: política de TLB inválida: '%s'. Use fifo ou lru.\n", argv[3]);
        return 1;
    }

    PoliticaPagina politicaPagina;
    if (strcmp(argv[2], "fifo") == 0) {
        politicaPagina = POLITICA_FIFO;
    }
    else {
        politicaPagina = POLITICA_LRU;
    }

    PoliticaTlb politicaTlb;
    if (strcmp(argv[3], "fifo") == 0) {
        politicaTlb = TLB_FIFO;
    }
    else {
        politicaTlb = TLB_LRU;
    }
    
    AddressList enderecos = {0};
    if (carregaEnderecos(argv[1], &enderecos) != 0) {
        liberaLista(&enderecos);
        return 1;
    }

    Memoria memoria;
    
    if (inicializaMemoria(&memoria, politicaPagina, politicaTlb) != 0) {
        liberaLista(&enderecos);
        return 1;
    }

    FILE *saida = fopen("correct.txt", "w");
    if (saida == NULL) {
        fprintf(stderr, "Falha ao abrir arquivo\n");
        liberaMemoria(&memoria);
        liberaLista(&enderecos);
        return 1;
    }
    
    if (executaSimulacao(&memoria, &enderecos, saida) != 0) {
        fclose(saida);
        liberaMemoria(&memoria);
        liberaLista(&enderecos);
        return 1;
    }

    fclose(saida);
    liberaMemoria(&memoria);
    liberaLista(&enderecos);
    
    return 0;
}

static StatusEndereco validaEndereco(const char *s, int *saida) {
    errno = 0;
    char *fim;
    long valor = strtol(s, &fim, 10);

    if (fim == s) {
        return ENDERECO_SEM_DIGITOS;
    }
    if (errno == ERANGE) {
        return ENDERECO_OVERFLOW;
    }
    while (isspace((unsigned char)*fim)) {
        fim++;
    }
    if (*fim != '\0') {
        return ENDERECO_LIXO;
    }
    if (valor < 0 || valor > 65535) {
        return ENDERECO_FORA_DA_FAIXA;
    }
    
    *saida = (int)valor;
    return ENDERECO_OK;
}

static const char *motivoEndereco(StatusEndereco status) {
    switch (status) {
        case ENDERECO_SEM_DIGITOS:
            return "nenhum número foi encontrado";
        case ENDERECO_OVERFLOW:
            return "número grande demais";
        case ENDERECO_LIXO:
            return "há caracteres inválidos após o número";
        case ENDERECO_FORA_DA_FAIXA:
            return "endereço fora da faixa 0..65535";
        case ENDERECO_OK:
            break;
    }
    return "endereço válido";
}

static int carregaEnderecos(const char *caminho, AddressList *lista) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) {
        fprintf(stderr, "Erro: não foi possível abrir '%s': %s\n", caminho, strerror(errno));
        return 1;
    }

    char linha[128];
    int numeroLinha = 0;
    int enderecosValidos = 0;

    while (fgets(linha, sizeof(linha), f) != NULL) {
        int endereco;

        numeroLinha++;
        StatusEndereco status = validaEndereco(linha, &endereco);
        if (status != ENDERECO_OK) {
            fprintf(stderr, "Erro: linha %d do arquivo '%s': %s.\n",
                    numeroLinha, caminho, motivoEndereco(status));
            fclose(f);
            return 1;
        }

        if (adicionarLista(lista, endereco) != 0) {
            fprintf(stderr, "Erro de memória\n");
            fclose(f);
            return 1;
        }
        enderecosValidos++;
    }

    if (ferror(f)) {
        fprintf(stderr, "Erro: falha de leitura no arquivo '%s'.\n", caminho);
        fclose(f);
        return 1;
    }

    fclose(f);

    if (enderecosValidos == 0) {
        fprintf(stderr, "Erro: o arquivo '%s' está vazio.\n", caminho);
        return 1;
    }
    
    return 0;
}

static int adicionarLista(AddressList *lista, int valor) {
    if (lista->quantidade == lista->capacidade) {
        size_t novaCapacidade = lista->capacidade == 0 ? 64 : lista->capacidade * 2;
        int *tmp = realloc(lista->itens, novaCapacidade * sizeof(*tmp));
        
        if (tmp == NULL) {
            return 1;
        }
        lista->itens = tmp;
        lista->capacidade = novaCapacidade;
    }

    lista->itens[lista->quantidade] = valor;
    lista->quantidade++;

    return 0;
}

static void liberaLista(AddressList *lista) {
    if (lista == NULL) {
        return;
    }
    free(lista->itens);
    lista->itens = NULL;
    lista->capacidade = 0;
    lista->quantidade = 0;
}

static int inicializaMemoria(Memoria *memoria, PoliticaPagina politicaPagina, PoliticaTlb politicaTlb) {
    for (int i = 0; i < NUM_PAGES; i++) {
        memoria->tabelaPaginas[i] = INVALID_FRAME;
        memoria->ultimoUso[i] = 0;
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        memoria->tlb[i].frame = INVALID_FRAME;
        memoria->tlb[i].pagina = INVALID_FRAME;
        memoria->tlb[i].valido = 0;
        memoria->tlb[i].ultimoUso = 0;
    }

    memoria->proximoFrame = 0;
    memoria->inicioFila = 0;
    memoria->fimFila = 0;
    memoria->tamanhoFila = 0;
    memoria->relogio = 0;
    memoria->relogioTlb = 0;
    memoria->politicaPagina = politicaPagina;
    memoria->politicaTlb = politicaTlb;
    memoria->tlbHits = 0;
    memoria->proximaEntradaTlb = 0;
    memoria->pageFaults = 0;

    memoria->backingStore = fopen("BACKING_STORE.bin", "rb");
    if (memoria->backingStore == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir BACKING_STORE.bin: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static void liberaMemoria(Memoria *memoria) {
    if (memoria->backingStore != NULL) {
        fclose(memoria->backingStore);
    }
    memoria->backingStore = NULL;
}

static int carregaPagina(Memoria *memoria, int pagina, int frame) {
    long deslocamento = (long)pagina * PAGE_SIZE;

    if (fseek(memoria->backingStore, deslocamento, SEEK_SET) != 0) {
        perror("fseek");
        return 1;
    }

    size_t lidos = fread(memoria->frames[frame], 1, PAGE_SIZE, memoria->backingStore);
    if (lidos != PAGE_SIZE) {
        perror("fread");
        return 1;
    }

    return 0;
}

static int traduzEndereco(Memoria *memoria, int enderecoVirtual, int *enderecoFisico, signed char *valor, int *indiceTlb) {
    int pagina = enderecoVirtual / PAGE_SIZE;
    int offset = enderecoVirtual % PAGE_SIZE;

    if (pagina < 0 || pagina >= NUM_PAGES) {
        return 1;
    }

    int slot = buscaTlb(memoria, pagina);
    if (slot != -1) {
        int frame = memoria->tlb[slot].frame;

        memoria->tlbHits++;
        registraUso(memoria, pagina);

        if (memoria->politicaTlb == TLB_LRU) {
            memoria->tlb[slot].ultimoUso = ++memoria->relogioTlb;
        }

        int fisico = frame * PAGE_SIZE + offset;
        unsigned char bruto = memoria->frames[frame][offset];

        *enderecoFisico = fisico;
        *valor = (signed char)bruto;
        *indiceTlb = slot;

        return 0;
    }
    
    int frame = memoria->tabelaPaginas[pagina];

    if (frame == INVALID_FRAME) {
        int vitima = INVALID_FRAME;
        memoria->pageFaults++;

        if (memoria->proximoFrame < NUM_FRAMES) {
            frame = memoria->proximoFrame;
            memoria->proximoFrame++;
        }
        else {
            vitima = escolheVitimaPagina(memoria);
            if (vitima == INVALID_FRAME) {
                fprintf(stderr, "Erro: politica de página não implementada\n");
                return 1;
            }
            frame = memoria->tabelaPaginas[vitima];

            if (frame == INVALID_FRAME) {
                fprintf(stderr, "Erro: pagina vitima sem frame valido\n");
                return 1;
            }
        }

        if (carregaPagina(memoria, pagina, frame) != 0) {
            fprintf(stderr, "Erro: falha ao carregar do backing store\n");
            return 1;
        }

        if (vitima != INVALID_FRAME) {
            memoria->tabelaPaginas[vitima] = INVALID_FRAME;
        }
        
        if (memoria->politicaPagina == POLITICA_FIFO) {
            enfileiraFifo(memoria, pagina);
        }

        memoria->tabelaPaginas[pagina] = frame;
    }

    *indiceTlb = insereTlb(memoria, pagina, frame);
    registraUso(memoria, pagina);
    
    int fisico = frame * PAGE_SIZE + offset;
    unsigned char bruto = memoria->frames[frame][offset];

    *enderecoFisico = fisico;
    *valor = (signed char)bruto;

    return 0;
}

static int executaSimulacao(Memoria *memoria, const AddressList *enderecos, FILE *saida) {
    int fisico;
    int indiceTlb;
    signed char valor;
    for (size_t i = 0; i < enderecos->quantidade; i++) {
        if (traduzEndereco(memoria, enderecos->itens[i], &fisico, &valor, &indiceTlb) != 0) {
            fprintf(stderr, "Erro: falha de traducao na linha %zu, no endereco: %d\n", i, enderecos->itens[i]);
            return 1;
        }
        fprintf(saida, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", enderecos->itens[i], indiceTlb, fisico, valor);
    }
    
    fprintf(saida, "Number of Translated Addresses = %zu\n", enderecos->quantidade);
    fprintf(saida, "Page Faults = %d\n", memoria->pageFaults);
    fprintf(saida, "Page Fault Rate = %.3f\n", (double)memoria->pageFaults / enderecos->quantidade);
    fprintf(saida, "TLB Hits = %d\n", memoria->tlbHits);
    fprintf(saida, "TLB Hit Rate = %.3f\n", (double)memoria->tlbHits / enderecos->quantidade);
    
    return 0;
}

static void enfileiraFifo(Memoria *memoria, int pagina) {
    memoria->filaFifo[memoria->fimFila] = pagina;
    memoria->fimFila = (memoria->fimFila + 1) % NUM_FRAMES;
    memoria->tamanhoFila++;
}

static int desenfileiraFifo(Memoria *memoria) {
    int pagina = memoria->filaFifo[memoria->inicioFila];
    memoria->inicioFila = (memoria->inicioFila + 1) % NUM_FRAMES;
    memoria->tamanhoFila--;

    return pagina;
}

static int escolheVitimaPagina(Memoria *memoria) {
    switch (memoria->politicaPagina) {
        case POLITICA_FIFO:
            return desenfileiraFifo(memoria);
        case POLITICA_LRU:
            return escolheVitimaLru(memoria);
    }

    return INVALID_FRAME;
}

static void registraUso(Memoria *memoria, int pagina) {
    if (memoria->politicaPagina == POLITICA_LRU) {
        memoria->relogio++;
        memoria->ultimoUso[pagina] = memoria->relogio;
    }
}

static int escolheVitimaLru(Memoria *memoria) {
    int vitima = INVALID_FRAME;
    unsigned long menosRecente = ULONG_MAX;
    
    for (int pagina = 0; pagina < NUM_PAGES; pagina++) {
        if (memoria->tabelaPaginas[pagina] == INVALID_FRAME) continue;
        if (memoria->ultimoUso[pagina] < menosRecente) {
            menosRecente = memoria->ultimoUso[pagina];
            vitima = pagina;
        }
    }

    return vitima;
}

static int buscaTlb(Memoria *memoria, int pagina) {
    pthread_t threads[TLB_SIZE];
    ThreadTlb buscas[TLB_SIZE];

    for (int i = 0; i < TLB_SIZE; i++) {
        buscas[i].memoria = memoria;
        buscas[i].indice = i;
        buscas[i].pagina = pagina;
        buscas[i].encontrou = 0;
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        pthread_create(&threads[i], NULL, procuraNaTlb, &buscas[i]);
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        if (buscas[i].encontrou) {
            return i;
        }
    }
    
    return -1;
}

static int insereTlb(Memoria *memoria, int pagina, int frame) {
    int slot;
    if (memoria->politicaTlb == TLB_FIFO) {
        slot = memoria->proximaEntradaTlb;
    }
    else {
        slot = escolheEntradaTlbLru(memoria);
    }
    
    memoria->tlb[slot].pagina = pagina;
    memoria->tlb[slot].frame = frame;
    memoria->tlb[slot].valido = 1;
    memoria->tlb[slot].ultimoUso = ++memoria->relogioTlb;

    memoria->proximaEntradaTlb = (memoria->proximaEntradaTlb + 1) % TLB_SIZE;

    return slot;
}

static int escolheEntradaTlbLru(Memoria *memoria) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!memoria->tlb[i].valido) return i;
    }

    unsigned long menor = memoria->tlb[0].ultimoUso;
    int idx = 0;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (memoria->tlb[i].ultimoUso < menor) {
            menor = memoria->tlb[i].ultimoUso;
            idx = i;
        }
    }

    return idx;
}

static void *procuraNaTlb(void *arg) {
    ThreadTlb *thread = arg;
    EntradaTlb *entrada = &thread->memoria->tlb[thread->indice];

    if (entrada->valido && entrada->pagina == thread->pagina) {
        thread->encontrou = 1;
    }

    return NULL;
}