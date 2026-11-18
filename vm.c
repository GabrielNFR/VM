#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128
#define INVALID_FRAME (-1)

typedef struct {
    unsigned char frames[NUM_FRAMES][PAGE_SIZE];
    int tabelaPaginas[NUM_PAGES];
    int proximoFrame;
    FILE *backingStore;
} Memoria;

typedef enum {
    ENDERECO_OK = 0,
    ENDERECO_SEM_DIGITOS,
    ENDERECO_OVERFLOW,
    ENDERECO_LIXO,
    ENDERECO_FORA_DA_FAIXA
} StatusEndereco;

typedef struct {
    int *itens;
    size_t quantidade;
    size_t capacidade;
} AddressList;

static StatusEndereco validaEndereco(const char *s, int *saida);
static void liberaLista(AddressList *lista);
static int carregaEnderecos(const char *caminho, AddressList *lista);
static int adicionarLista(AddressList *lista, int valor);
static int inicializaMemoria(Memoria *memoria);
static void liberaMemoria(Memoria *memoria);
static int carregaPagina(Memoria *memoria, int pagina, int frame);
static int traduzEndereco(Memoria *memoria, int enderecoVirtual, int *enderecoFisico, signed char *valor);

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

    AddressList enderecos = {0};
    if (carregaEnderecos(argv[1], &enderecos) != 0) {
        liberaLista(&enderecos);
        return 1;
    }
    printf("Enderecos validados: %zu\nPrimeiro endereco: %d\nUltimo endereco: %d\n", enderecos.quantidade, enderecos.itens[0], enderecos.itens[enderecos.quantidade - 1]);

    Memoria memoria;
    if (inicializaMemoria(&memoria) != 0) {
        liberaLista(&enderecos);
        return 1;
    }

    int fisico; 
    signed char valor;

    if (traduzEndereco(&memoria, enderecos.itens[0], &fisico, &valor) != 0) {
        fprintf(stderr, "Erro: falha na tradução do primeiro endereço\n");
        liberaMemoria(&memoria);
        liberaLista(&enderecos);
        return 1;
    }

    printf("Endereco logico: %d\n", enderecos.itens[0]);
    printf("Endereco fisico: %d\n", fisico);
    printf("Valor: %d\n", valor);

    int fisico2;
    signed char valor2;

    if (traduzEndereco(&memoria, enderecos.itens[0], &fisico2, &valor2) != 0) {
        fprintf(stderr, "Erro: falha na segunda tradução\n");
        liberaMemoria(&memoria);
        liberaLista(&enderecos);
        return 1;
    }
    
    printf("Segunda traducao: fisico = %d, valor = %d\n", fisico2, valor2);
    
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

static int inicializaMemoria(Memoria *memoria) {
    for (int i = 0; i < NUM_PAGES; i++) {
        memoria->tabelaPaginas[i] = INVALID_FRAME;
    }

    memoria->proximoFrame = 0;

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

static int traduzEndereco(Memoria *memoria, int enderecoVirtual, int *enderecoFisico, signed char *valor) {
    int pagina = enderecoVirtual / PAGE_SIZE;
    int offset = enderecoVirtual % PAGE_SIZE;

    if (pagina < 0 || pagina >= NUM_PAGES) {
        return 1;
    }

    int frame = memoria->tabelaPaginas[pagina];

    if (frame == -1) {
        if (memoria->proximoFrame >= NUM_FRAMES) {
            return 1;
        }

        frame = memoria->proximoFrame;

        if (carregaPagina(memoria, pagina, frame) != 0) {
            fprintf(stderr, "Erro: falha ao carregar do backing store\n");
            return 1;
        }

        memoria->tabelaPaginas[pagina] = frame;
        memoria->proximoFrame++;
    }

    int fisico = frame * PAGE_SIZE + offset;
    unsigned char bruto = memoria->frames[frame][offset];

    *enderecoFisico = fisico;
    *valor = (signed char)bruto;

    return 0;
}