# 1. Objetivo
O enunciado pede um simulador de gerenciador de memória virtual em C, com 128 frames, TLB de 16 entradas, tabela de páginas para 256 páginas, paginação por demanda sobre BACKING_STORE.bin, substituição FIFO e LRU para página e para TLB, busca na TLB com uma thread por entrada, três argumentos de linha de comando, saída correct.txt e validação dos parâmetros e do arquivo de endereços.

# 2. Checklist de Requisitos
Um requisito por linha. Sem parágrafo, se precisar explicar algo, use a coluna "como testar". Status: OK, Parcial ou Não feito.
| Requisito do enunciado | Status | Como testar / evidência |
|:-----------------------|:-------|:------------------------|
| Programa em C, executável em Linux, Unix ou macOS, compilado com make e gerando vm | OK | make clean e make geraram vm em 2026-11-17 22:04:05 |
| Três argumentos de linha de comando: arquivo, política de página e política de TLB | Parcial | A política de página é usada em 2026-11-18 22:44:07, mas a política de TLB ainda não é aplicada |
| Política de substituição de página FIFO | OK | 2026-11-18 22:44:07, execução completa com 1000 traduções e nenhuma ocorrência de memória cheia |
| Política de substituição de página LRU | Não feito | 2026-11-18 22:44:07, interrupção controlada no endereço 48855 porque o LRU ainda não foi implementado |
| Política de substituição de TLB FIFO | Não feito | Sem implementação |
| Política de substituição de TLB LRU | Não feito | Sem implementação |
| Memória física com 128 frames | OK | A struct Memoria define 128 frames e a execução completa com substituição ocorreu em 2026-11-18 22:44:07 |
| Preencher os frames de 0 a 127 e substituir quando a memória encher | Parcial | O FIFO preenche e substitui frames, mas o LRU ainda não existe em 2026-11-18 22:44:07 |
| Traduzir endereços lógicos em endereços físicos | OK | 2026-11-18 22:44:07, execução completa com 1000 traduções |
| Ler páginas do BACKING_STORE.bin durante page faults | OK | 2026-11-18 22:44:07, execução completa com page faults e substituição |
| Buscar a TLB com uma thread para cada entrada | Não feito | Sem implementação |
| Gerar correct.txt no formato do exemplo | Não feito | Sem implementação |
| Validar número e tipo de argumentos com erro coerente | Parcial | Implementado no código, mas o log não registra execução com argumento inválido |
| Validar arquivo inexistente, ilegível ou em formato diferente do exemplo | OK | Log de 2026-11-17 22:04:12 registra linha inválida, valor fora de faixa, arquivo vazio e arquivo inexistente |
| Ler, validar e armazenar os 1000 endereços do arquivo de exemplo | OK | Log de 2026-11-18 20:18:36 mostra 1000 endereços validados, com primeiro 16916 e último 12107 |

# 3. Como Reproduzir
Comandos reais, copiáveis. Se não rodar com o que está aqui, não é avaliado.
Compilar:
```bash
make clean
make
```
Executar:
```bash
./vm addresses.txt fifo fifo
```
# 4. Arquitetura (resumida)
Uma linha por arquivo, só o que é específico do seu projeto.

**Arquivos e responsabilidades:**

1. *vm.c* -> valida argumentos, carrega e armazena endereços, inicializa a memória, seleciona a política de página e traduz todos os endereços com page faults e substituição FIFO, ainda sem TLB e sem LRU
2. *Makefile* -> compila vm.c com gcc, -pthread, -Wall, -Wextra e -Werror
3. *addresses.txt* -> 1000 endereços lógicos carregados e armazenados
4. *BACKING_STORE.bin* -> lido nos page faults para carregar páginas nos frames
5. *correct.txt* -> referência de saída para LRU de página e FIFO de TLB, ainda não gerada
6. *evidencias.log* -> log das sessões de compilação e testes

**Decisões técnicas:**
1. *Usar strtol com errno e endptr* -> validar formato, overflow e faixa -> endereços com lixo ou fora de 0..65535 são rejeitados
2. *Separar StatusEndereco, motivoEndereco e validaArquivo* -> evitar mensagens duplicadas -> cada erro gera uma única mensagem com linha e motivo
3. *Agrupar frames, tabela de páginas, próximo frame e backing store em Memoria* -> centralizar o estado da simulação -> a tradução recebe apenas um ponteiro para a memória
4. *Inicializar a tabela com INVALID_FRAME (-1)* -> o frame 0 é um frame válido -> evita falso page fault na primeira página
5. *Usar unsigned char nos frames e signed char no valor* -> ler bytes brutos do backing store e interpretar o valor final como assinado -> valores negativos do correct.txt podem ser representados
6. *Separar PoliticaPagina e escolheVitimaPagina* -> manter a tradução independente da política -> o FIFO funciona e o LRU pode ser adicionado sem alterar o fluxo principal
7. *Usar uma fila circular de páginas para FIFO* -> guardar a ordem de carregamento -> a vítima é a página carregada há mais tempo
8. *Deixar o LRU como erro controlado nesta etapa* -> o algoritmo ainda não existe -> o programa não gera resultado incorreto de forma silenciosa
9. *Manter saída de depuração por endereço* -> validar as 1000 traduções antes do formato final -> correct.txt ainda não é gerado


# 5. Estratégias e Diário de Desenvolvimento
Isto é o núcleo do relatório.
## 5.1 Estratégias da semana
Uma linha por abordagem diferente que você tentou. Se você não mudou de abordagem, preencha só S1.
| Estratégia | Nome curto | Contexto | Motivo da troca (se houve) |
|:-----------|:-----------|:---------|:---------------------------|
| S1 | Validação incremental | Implementar argumentos, leitura e validação do arquivo antes da simulação | Não houve troca |
| S2 | Validação separada da mensagem | Duas mensagens para o mesmo erro na primeira versão | Separar responsabilidades e emitir uma única mensagem contextual |
| S3 | Tradução incremental da memória | Implementar page fault, frame livre e leitura do backing store antes de TLB e substituição | Não houve troca |
| S4 | Substituição FIFO antes do LRU | Implementar a política de substituição mais simples primeiro e isolar a escolha de vítima | Não houve troca |

## 5.2 Diário de Tentativas
Uma linha por tentativa relevante, marcando a qual estratégia (S1/S2/S3) ela pertence, obviamente precisa ser em ordem cronológica. A coluna "Quando" precisa bater com a ordem cronológica do evidencias.log, é isso que comprova que o fluxo é real. "Hipótese/Causa" só é preenchida quando o resultado foi falha ou parcial.
| # | Estrat. | O que tentei | Resultado | Hipótese/Causa (se falhou) | Quando | Evidência |
|:--|:--------|:-------------|:----------|:---------------------------|:-------|:----------|
| 1 | S1 | Compilar com make e conferir a geração de vm | OK | - | 2026-11-17 19:18:21 | 2026-11-17 19:18:21 |
| 2 | S1 | Ler e validar apenas a primeira linha do arquivo | OK | - | 2026-11-17 19:18:21 | 2026-11-17 19:18:21 |
| 3 | S1 | Testar erros na primeira linha | Parcial | O validador e o leitor imprimiam mensagens separadas para o mesmo erro | 2026-11-17 19:18:21 | 2026-11-17 19:18:21 |
| 4 | S1 | Validar o arquivo inteiro e contar os endereços | OK | - | 2026-11-17 22:04:12 | 2026-11-17 22:04:12 |
| 5 | S1 | Testar linha inválida, valor fora de faixa, arquivo vazio e arquivo inexistente | Parcial | As mensagens ainda saíam duplicadas na primeira versão | 2026-11-17 22:04:12 | 2026-11-17 22:04:12 |
| 6 | S2 | Refatorar a validação para retornar status e imprimir uma única mensagem com linha e motivo | OK | - | 2026-11-17 22:04:12 | 2026-11-17 22:04:12 |
| 7 | S1 | Armazenar os endereços em vetor dinâmico | Falha | A capacidade inicial foi definida sem alocar itens, então a escrita ocorreu em ponteiro NULL | 2026-11-18 14:58:56 | 2026-11-18 14:58:56 |
| 8 | S1 | Corrigir a alocação inicial e validar o armazenamento | OK | - | 2026-11-18 20:18:36 | 2026-11-18 20:18:36 |
| 9 | S3 | Traduzir o primeiro endereço com page fault e frame livre | OK | - | 2026-11-18 20:18:36 | 2026-11-18 20:18:36 |
| 10 | S3 | Executar FIFO completo e conferir ausência de memória cheia | OK | - | 2026-11-18 22:44:07 | 2026-11-18 22:44:07 |
| 11 | S3 | Testar a seleção de política após a mudança no page fault | Falha | A verificação de vítima também rodava no caminho de frame livre e a página era enfileirada duas vezes | 2026-11-18 22:44:07 | 2026-11-18 22:44:07 |
| 12 | S3 | Corrigir o tratamento da vítima e da fila FIFO | OK | - | 2026-11-18 22:44:07 | 2026-11-18 22:44:07 |
| 13 | S4 | Validar a seleção de política e o erro controlado do LRU | Parcial | O LRU ainda não foi implementado e interrompe a execução quando a memória enche | 2026-11-18 22:44:07 | 2026-11-18 22:44:07 |

Regra prática: se algo deu muito errado (bug que te travou por horas), a linha da tabela resume em 1 frase e aponta para o trecho do evidencias.log com o detalhe.

# 6. Evidências
A regra continua: toda afirmação relevante precisa de prova. O que muda é como capturar essa prova, para não depender de print manual toda hora.

## 6.1 Log automático (faz o trabalho pesado por você)
No início da sessão de trabalho, rode:
```
script -a evidencias.log
```
Isso grava automaticamente todos os comandos e saídas do terminal num arquivo, sem você precisar tirar print de cada linha. Comece a sessão com date, whoami e pwd, assim o log já tem timestamp, usuário e diretório sem esforço extra. **O evidencias.log deve ser enviado dentro do .tar.**

## 6.2 Prints

- Erro principal: o momento do bug mais relevante da semana.
- Validação final: teste passando / saída correta.

Prints extras só se algo excepcional aconteceu e o log sozinho não explica, você precisa ter esse feeling, mas posso dar uma ajuda (ex.: comportamento visual, crash com stack trace longo).

# 7. Uso de IA
Obrigatório preencher mesmo se a resposta for "não utilizei".
**Onde usei IA (2–3 linhas):** 

**O que validei manualmente e como:** 

# 8. Reflexão Final (5–8 linhas)
O que você melhoraria com mais tempo, principais decisões técnicas e erros que evitaria na próxima. Tudo junto, sem subseções separadas.

# 9. Checklist Final de Entrega

| Item | Confirmado? |
|:-----|:------------|
| Compilei do zero seguindo só o que está no relatório |  |
| Rodei os testes e evidencias.log foi gerado |  |
| Tenho prints obrigatórios |  |
| Testei pelo menos um caso limite e um caso inválido |  |
| Preenchi a seção de uso de IA |  |
| Revisei o relatório e removi frases genéricas / vazias |  |

# 10. Se eu tivesse mais 2 horas

Aplique as instruções abaixo de forma objetiva e verificável, evitando texto genérico.
Escreva 5-10 linhas sobre o que você melhoraria (robustez, testes, warnings, performance, organização).

# Regras de Integridade (não mudam)
- Ocorrência relevante sem evidência (log ou print) = não conta para a nota.
- Contradição entre prints, log e relato = relatório perde credibilidade.
- Indício de evidência fabricada = entrega invalidada.
