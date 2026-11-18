#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    ENDERECO_OK = 0,
    ENDERECO_SEM_DIGITOS,
    ENDERECO_OVERFLOW,
    ENDERECO_LIXO,
    ENDERECO_FORA_DA_FAIXA
} StatusEndereco;

static StatusEndereco validaEndereco(const char *s, int *saida);
static int validaArquivo(const char *caminho, int *quantidade);

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

    int quantidade = 0;
    if (validaArquivo(argv[1], &quantidade) != 0) {
        return 1;
    }
    printf("Endereços validados: %d\n", quantidade);

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

static int validaArquivo(const char *caminho, int *quantidade) {
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
    *quantidade = enderecosValidos;
    
    return 0;
}