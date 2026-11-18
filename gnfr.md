# 1. Objetivo
O enunciado pede um simulador de gerenciador de memória virtual em C, com 128 frames, TLB de 16 entradas, tabela de páginas para 256 páginas, paginação por demanda sobre BACKING_STORE.bin, substituição FIFO e LRU para página e para TLB, busca na TLB com uma thread por entrada, três argumentos de linha de comando, saída correct.txt e validação dos parâmetros e do arquivo de endereços.


# 2. Checklist de Requisitos
Um requisito por linha. Sem parágrafo, se precisar explicar algo, use a coluna "como testar". Status: OK, Parcial ou Não feito.
| Requisito do enunciado | Status | Como testar / evidência |
|:-----------------------|:-------|:------------------------|
| Programa em C, executável em Linux, Unix ou macOS, compilado com make e gerando vm | OK | make clean e make geraram vm em 2026-11-17 22:04:05 |
| Três argumentos de linha de comando: arquivo, política de página e política de TLB | Parcial | ./vm addresses.txt fifo lru foi aceito em 2026-11-17 22:04:12, mas as políticas ainda não são usadas na simulação |
| Política de substituição de página FIFO | Não feito | Sem implementação |
| Política de substituição de página LRU | Não feito | Sem implementação |
| Política de substituição de TLB FIFO | Não feito | Sem implementação |
| Política de substituição de TLB LRU | Não feito | Sem implementação |
| Memória física com 128 frames | Não feito | Sem implementação |
| Preencher os frames de 0 a 127 e substituir quando a memória encher | Não feito | Sem implementação |
| Traduzir endereços lógicos em endereços físicos | Não feito | O programa atual apenas valida e conta os endereços |
| Ler páginas do BACKING_STORE.bin durante page faults | Não feito | ainda não é lido pelo programa |
| Buscar a TLB com uma thread para cada entrada | Não feito | Sem implementação |
| Gerar correct.txt no formato do exemplo | Não feito | Sem implementação |
| Validar número e tipo de argumentos com erro coerente | Ok | Implementado, mas falta gerar evidência |
| Validar arquivo inexistente, ilegível ou em formato diferente do exemplo | OK | Log de 2026-11-17 22:04:12 registra linha inválida, valor fora de faixa, arquivo vazio e arquivo inexistente |
| Ler e validar os 1000 endereços do arquivo de exemplo | Ok | Log de 2026-11-17 22:04:12 |

# 3. Como Reproduzir
Comandos reais, copiáveis. Se não rodar com o que está aqui, não é avaliado.
Compilar:
```bash
make clean
make
```
Executar:
```bash
./vm addresses.txt fifo lru
```
Saída atual esperada:
```text
Endereços validados: 1000
```
Casos inválidos usados nos testes:
```bash
printf '123\nabc\n' > invalido.txt
./vm invalido.txt fifo lru

printf '123\n70000\n' > invalido2.txt
./vm invalido2.txt fifo lru

: > vazio.txt
./vm vazio.txt fifo lru

./vm nao_existe.txt fifo lru
```
A versão atual ainda não gera o arquivo correct.txt.

# 4. Arquitetura (resumida)
Uma linha por arquivo, só o que é específico do seu projeto.

**Arquivos e responsabilidades:**

1. *vm.c* -> valida argumentos, lê e valida o arquivo de endereços, ainda sem simulação de memória
2. *Makefile* -> compila vm.c com gcc, -pthread, -Wall, -Wextra e -Werror
3. *addresses.txt* -> 1000 endereços lógicos usados na validação
4. *BACKING_STORE.bin* -> armazenamento de páginas previsto, ainda não lido pelo programa
5. *correct.txt* -> referência de saída para LRU de página e FIFO de TLB, ainda não gerada
6. *evidencias.log* -> log das sessões de compilação e testes

**Decisões técnicas:**
1. *Usar strtol com errno e endptr* -> validar formato, overflow e faixa -> endereços com lixo ou fora de 0..65535 são rejeitados
2. *Separar StatusEndereco, motivoEndereco e validaArquivo* -> evitar mensagens duplicadas -> cada erro gera uma única mensagem com linha e motivo


# 5. Estratégias e Diário de Desenvolvimento
Isto é o núcleo do relatório.
## 5.1 Estratégias da semana
Uma linha por abordagem diferente que você tentou. Se você não mudou de abordagem, preencha só S1.
| Estratégia | Nome curto | Contexto | Motivo da troca (se houve) |
|:-----------|:-----------|:---------|:---------------------------|
| S1 | Validação incremental | Implementar argumentos, leitura e validação do arquivo antes da simulação | Não houve troca |
| S2 | Validação separada da mensagem | Duas mensagens para o mesmo erro na primeira versão | Separar responsabilidades e emitir uma única mensagem contextual |

## 5.2 Diário de Tentativas
Uma linha por tentativa relevante, marcando a qual estratégia (S1/S2/S3) ela pertence, obviamente precisa ser em ordem cronológica. A coluna "Quando" precisa bater com a ordem cronológica do evidencias.log, é isso que comprova que o fluxo é real. "Hipótese/Causa" só é preenchida quando o resultado foi falha ou parcial.
| # | Estrat. | O que tentei | Resultado | Hipótese/Causa (se falhou) | Quando | Evidência |
|:--|:--------|:-------------|:----------|:---------------------------|:-------|:----------|
| 1 | S1 | Compilar com make e conferir a geração de vm | OK | - | 2026-11-17 19:18:21 | log linhas 2-14 |
| 2 | S1 | Ler e validar apenas a primeira linha do arquivo | OK | - | 2026-11-17 19:18:21 | log linha 25, primeiro endereço 16916 |
| 3 | S1 | Testar erros na primeira linha | Parcial | O validador e o leitor imprimiam mensagens separadas para o mesmo erro | 2026-11-17 19:18:21 | log linhas 30-31 |
| 4 | S1 | Validar o arquivo inteiro e contar os endereços | OK | - | 2026-11-17 22:04:12 | log linha 79, 1000 endereços validados |
| 5 | S1 | Testar linha inválida, valor fora de faixa, arquivo vazio e arquivo inexistente | Parcial | As mensagens ainda saíam duplicadas na primeira versão | 2026-11-17 22:04:12 | log linhas 85-86, 93-94, 97-98 e 101 |
| 6 | S2 | Refatorar a validação para retornar status e imprimir uma única mensagem com linha e motivo | OK | - | 2026-11-17 22:04:12 | log linhas 102-129, mensagens únicas nas linhas 115, 121, 126 e 129 |

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
