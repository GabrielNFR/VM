# Simulador de Gerenciador de Memória Virtual

Implementação em C do projeto **Designing a Virtual Memory Manager** (Silberschatz, *Operating System Concepts*, 10a ed., P-51), com as adaptações pedidas no enunciado: política de substituição de página e de TLB escolhidas por argumento, busca na TLB com uma thread por entrada e validação completa dos parâmetros de entrada.

O simulador lê uma lista de endereços lógicos de 16 bits, traduz cada um para um endereço físico de 8 bits e grava o resultado, junto com as estatísticas da simulação, no arquivo `correct.txt`.

## Sumário

- [Parâmetros da simulação](#parâmetros-da-simulação)
- [Requisitos](#requisitos)
- [Compilação](#compilação)
- [Execução](#execução)
- [Formato da saída](#formato-da-saída)
- [Resultados obtidos](#resultados-obtidos)
- [Arquitetura do código](#arquitetura-do-código)
- [Detalhes de implementação](#detalhes-de-implementação)
- [Tratamento de erros](#tratamento-de-erros)
- [Arquivos do projeto](#arquivos-do-projeto)
- [Testes manuais](#testes-manuais)
- [Limitações conhecidas](#limitações-conhecidas)
- [Referência](#referência)

## Parâmetros da simulação

| Parâmetro | Valor |
|:----------|:------|
| Tamanho da página | 256 bytes (`PAGE_SIZE`) |
| Número de páginas | 256 (`NUM_PAGES`) |
| Espaço de endereçamento lógico | 0 a 65535 (16 bits) |
| Tamanho da memória física | 128 frames (`NUM_FRAMES`) |
| Entradas da TLB | 16 (`TLB_SIZE`) |
| Fonte das páginas | `BACKING_STORE.bin` |
| Endereços do exemplo | 1000 linhas em `addresses.txt` |

O endereço lógico é dividido em número de página (8 bits mais significativos) e deslocamento (8 bits menos significativos). O endereço físico é formado pelo número do frame (7 bits) e pelo mesmo deslocamento.

## Requisitos

- Compilador C com suporte a POSIX threads (`gcc` ou `clang`)
- `make`
- Ambiente Linux, Unix ou macOS

## Compilação

```bash
make clean
make
```

O `Makefile` usa as seguintes flags:

```
-std=gnu11 -Wall -Wextra -Werror -Wno-unused-parameter -O2 -g -pthread
```

Como `-Werror` está ativo, qualquer aviso do compilador interrompe o build. O resultado é o executável `vm` no diretório atual.

## Execução

```bash
./vm <arquivo-de-enderecos> <fifo|lru> <fifo|lru>
```

- **Argumento 1**: caminho do arquivo com um endereço lógico por linha
- **Argumento 2**: política de substituição de **página** (`fifo` ou `lru`)
- **Argumento 3**: política de substituição de **TLB** (`fifo` ou `lru`)

Exemplos:

```bash
./vm addresses.txt fifo fifo
./vm addresses.txt fifo lru
./vm addresses.txt lru fifo
./vm addresses.txt lru lru
```

O programa precisa ser executado no mesmo diretório de `BACKING_STORE.bin`, pois o arquivo é aberto por caminho relativo. A saída é sempre gravada em `correct.txt` no diretório atual, sobrescrevendo a execução anterior.

## Formato da saída

Uma linha por endereço traduzido, seguida do resumo:

```
Virtual address: <logico> TLB: <indice> Physical address: <fisico> Value: <valor>
```

- `Virtual address`: endereço lógico lido do arquivo
- `TLB`: índice da entrada da TLB usada na tradução (0 a 15)
- `Physical address`: endereço físico calculado
- `Value`: byte lido no frame, interpretado como `signed char`

Primeiras linhas e resumo de uma execução com `lru fifo`:

```
Virtual address: 16916 TLB: 0 Physical address: 20 Value: 0
Virtual address: 62493 TLB: 1 Physical address: 285 Value: 0
Virtual address: 30198 TLB: 2 Physical address: 758 Value: 29
...
Virtual address: 12107 TLB: 1 Physical address: 17739 Value: -46
Number of Translated Addresses = 1000
Page Faults = 539
Page Fault Rate = 0.539
TLB Hits = 54
TLB Hit Rate = 0.054
```

As taxas são calculadas sempre sobre o total de endereços traduzidos, e não sobre o total de acessos menos os hits.

## Resultados obtidos

Execuções com `addresses.txt`, 1000 traduções em todas as combinações:

| Página | TLB | Page Faults | Page Fault Rate | TLB Hits | TLB Hit Rate |
|:-------|:----|:------------|:----------------|:---------|:-------------|
| fifo | fifo | 538 | 0.538 | 54 | 0.054 |
| fifo | lru | 538 | 0.538 | 55 | 0.055 |
| lru | fifo | 539 | 0.539 | 54 | 0.054 |
| lru | lru | 539 | 0.539 | 55 | 0.055 |

A combinação `lru fifo` reproduz exatamente a saída de referência do enunciado. A combinação `lru lru` produz os mesmos endereços físicos e os mesmos valores, mudando apenas o índice da entrada de TLB e a contagem de hits. Isso é esperado, porque a referência do enunciado usa FIFO de TLB.

Grupos de funções:

**Entrada e validação**

- `validaEndereco`: converte a linha com `strtol` e classifica o resultado
- `motivoEndereco`: traduz o `StatusEndereco` em texto para a mensagem de erro
- `carregaEnderecos`: lê o arquivo linha a linha, valida e armazena
- `adicionarLista` e `liberaLista`: gerenciam o vetor dinâmico de endereços

**Memória**

- `inicializaMemoria`: zera tabela de páginas, TLB, fila e relógios, e abre o backing store
- `liberaMemoria`: fecha o backing store
- `carregaPagina`: lê 256 bytes do backing store para um frame
- `traduzEndereco`: implementa o fluxo completo de tradução

**Substituição de página**

- `enfileiraFifo` e `desenfileiraFifo`: fila circular de páginas para o FIFO
- `escolheVitimaLru`: varre a tabela e escolhe a página com menor recência
- `escolheVitimaPagina`: despacha para a política configurada
- `registraUso`: atualiza a recência quando a política é LRU

**TLB**

- `buscaTlb`: cria 16 threads, uma por entrada, e espera todas com `pthread_join`
- `procuraNaTlb`: rotina executada por cada thread, apenas leitura da TLB
- `insereTlb`: insere a tradução no próximo índice (FIFO) ou na entrada menos recente (LRU)
- `escolheEntradaTlbLru`: usa entrada livre antes de considerar substituição

## Detalhes de implementação

### Validação de endereços

Cada linha passa por `strtol` com checagem de `errno`, de `endptr` e de faixa. Os casos rejeitados são:

| Situação | Mensagem |
|:---------|:---------|
| Linha sem dígitos | `nenhum número foi encontrado` |
| Valor maior que `long` | `número grande demais` |
| Caracteres após o número | `há caracteres inválidos após o número` |
| Valor fora de 0 a 65535 | `endereço fora da faixa 0..65535` |

O arquivo inteiro é validado e armazenado antes de qualquer tradução. Assim, um erro no meio do arquivo não gera saída parcial em `correct.txt`.

### Uso de `INVALID_FRAME`

A tabela de páginas é inicializada com `-1` em vez de `0`, porque o frame `0` é um frame válido. Sem isso, a primeira página carregada seria confundida com uma página ausente.

### Representação de bytes

Os frames são `unsigned char`, porque os bytes vêm crus do backing store. O valor exibido na saída é `signed char`, para que valores acima de 127 apareçam como negativos, igual à referência.

### Fluxo de tradução

Ordem de consulta em `traduzEndereco`:

1. Busca paralela na TLB
2. Em caso de hit, usa o frame da TLB, incrementa `tlbHits` e atualiza a recência da entrada
3. Em caso de miss, consulta a tabela de páginas
4. Se a página for inválida, incrementa `pageFaults`, escolhe um frame livre ou uma vítima e carrega a página
5. Insere a tradução na TLB
6. Registra o uso da página para o LRU
7. Calcula o endereço físico e lê o byte

### FIFO de página

Fila circular de `NUM_FRAMES` posições com `inicioFila`, `fimFila` e `tamanhoFila`. Cada page fault enfileira a página carregada. Quando a memória está cheia, a vítima é a página no início da fila, ou seja, a carregada há mais tempo.

O enfileiramento acontece uma única vez por page fault e apenas quando a política é FIFO. Chamar `enfileiraFifo` duas vezes corrompe o estado da fila.

### LRU de página

Cada acesso a uma página válida grava `++relogio` em `ultimoUso[pagina]`. A vítima é a página presente na tabela com o menor `ultimoUso`. A comparação precisa ser feita sobre a recência, e não sobre o conteúdo da tabela de páginas, que guarda o frame.

### TLB com threads

`buscaTlb` cria `TLB_SIZE` threads, cada uma responsável por uma entrada. Cada thread recebe uma `ThreadTlb` com ponteiro para a memória, o índice da entrada e a página procurada. A sincronização é feita com `pthread_join`, sem mutex, porque a TLB é somente leitura durante a busca e cada thread escreve no seu próprio resultado.

```c
typedef struct {
    Memoria *memoria;
    int pagina;
    int indice;
    int encontrou;
} ThreadTlb;
```

### TLB FIFO e TLB LRU

- **FIFO**: `proximaEntradaTlb` avança circularmente a cada inserção
- **LRU**: `escolheEntradaTlbLru` usa a primeira entrada inválida e, se não houver, a de menor `ultimoUso`

A inserção na TLB acontece uma única vez por tradução com miss, e o índice retornado é o que aparece no campo `TLB` da saída.

## Tratamento de erros

Todos os erros vão para `stderr` e retornam código de saída `1`. As mensagens indicam o problema concreto, sem texto genérico.

| Situação | Mensagem |
|:---------|:---------|
| Número de argumentos diferente de 3 | `Erro: uso: ./vm <arquivo> <fifo\|lru> <fifo\|lru>` |
| Política de página desconhecida | `Erro: política de página inválida: '<valor>'. Use fifo ou lru.` |
| Política de TLB desconhecida | `Erro: política de TLB inválida: '<valor>'. Use fifo ou lru.` |
| Arquivo inexistente | `Erro: não foi possível abrir '<arquivo>': <motivo>` |
| Linha inválida | `Erro: linha <n> do arquivo '<arquivo>': <motivo>.` |
| Arquivo vazio | `Erro: o arquivo '<arquivo>' está vazio.` |
| Falha de leitura | `Erro: falha de leitura no arquivo '<arquivo>'.` |
| `BACKING_STORE.bin` ausente | `Erro: nao foi possivel abrir BACKING_STORE.bin: <motivo>` |
| Falha ao criar `correct.txt` | `Falha ao abrir arquivo` |
| Falha de tradução | `Erro: falha de traducao na linha <i>, no endereco: <valor>` |
| Falha de alocação | `Erro de memória` |

Exemplos de uso inválido:

```bash
./vm
./vm addresses.txt
./vm addresses.txt fifo
./vm addresses.txt aleatorio fifo
./vm addresses.txt fifo aleatorio
./vm naoexiste.txt fifo fifo
```

## Arquivos do projeto

| Arquivo | Papel |
|:--------|:------|
| `vm.c` | código-fonte completo do simulador |
| `Makefile` | compilação com `gcc`, `-pthread` e `-Werror` |
| `addresses.txt` | 1000 endereços lógicos de entrada |
| `BACKING_STORE.bin` | 65536 bytes de memória secundária, lidos nos page faults |
| `correct.txt` | saída gerada pela última execução |
| `gnfr.md` | relatório do projeto |
| `evidencias.log` | log das sessões de compilação e teste |

Os arquivos `saida_*.txt` são saídas de depuração de versões anteriores e não são usados pelo programa.

## Testes manuais

Compilação limpa:

```bash
make clean
make
```

Execução completa:

```bash
./vm addresses.txt lru fifo
```

Conferência do resumo:

```bash
tail -5 correct.txt
```

Comparação com a saída de referência do enunciado, guardada à parte:

```bash
diff -u correct_esperado.txt correct.txt
cmp -s correct_esperado.txt correct.txt && echo identicos
```

Teste de argumentos inválidos, esperando código de saída diferente de zero:

```bash
./vm addresses.txt fifo
echo "codigo: $?"
```

Teste de arquivo com linha inválida, com um arquivo temporário contendo `abc` ou `70000`:

```bash
printf '70000\n' > /tmp/invalido.txt
./vm /tmp/invalido.txt fifo fifo
echo "codigo: $?"
```

## Limitações conhecidas

- A TLB não é invalidada quando uma página é substituída. Uma entrada antiga continua apontando para o frame que agora pertence a outra página. O resultado do exemplo continua igual à referência porque a referência tem o mesmo comportamento, mas a implementação não é segura para sequências de acesso arbitrárias.
- A busca na TLB cria e destrói 16 threads por tradução, o que é correto do ponto de vista do enunciado e caro do ponto de vista de desempenho.
- A saída é sempre `correct.txt` no diretório atual, sem opção de caminho alternativo.
- A execução exige `BACKING_STORE.bin` no diretório atual.

## Referência

SILBERSCHATZ, Abraham. *Operating System Concepts*. 10. ed. Projeto de programação P-51, Designing a Virtual Memory Manager.
