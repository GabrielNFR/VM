#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Erro: uso: %s <arquivo> <fifo|lru> <fifo|lru>\n", argv[0]);
        return 1;
    }
}