# 1. Objetivo
O que o enunciado pede e o que você entrega. Não explique a implementação aqui.

# 2. Checklist de Requisitos
Um requisito por linha. Sem parágrafo, se precisar explicar algo, use a coluna "como testar". Status: OK, Parcial ou Não feito.
| Requisito do enunciado | Status | Como testar / evidência |
|:-----------------------|:-------|:------------------------|
|  |  |  |


# 3. Como Reproduzir
Comandos reais, copiáveis. Se não rodar com o que está aqui, não é avaliado.
Compilar:
```
```
Executar:
```
```
# 4. Arquitetura (resumida)
Uma linha por arquivo, só o que é específico do seu projeto.

**Arquivos e responsabilidades:**

1. *[nome do arquivo]* -> *[o que ele faz, em uma linha]*
2. *[nome do arquivo]* -> *[o que ele faz, em uma linha]*
3. *[nome do arquivo]* -> *[o que ele faz, em uma linha]*
4. *[nome do arquivo]* -> *[o que ele faz, em uma linha]*

**Decisões técnicas:**
1. *[decisão]* -> *[motivo]* -> *[consequência]*
2. *[decisão]* -> *[motivo]* -> *[consequência]*
3. *[decisão]* -> *[motivo]* -> *[consequência]*
4. *[decisão]* -> *[motivo]* -> *[consequência]*

# 5. Estratégias e Diário de Desenvolvimento
Isto é o núcleo do relatório.
## 5.1 Estratégias da semana
Uma linha por abordagem diferente que você tentou. Se você não mudou de abordagem, preencha só S1.
| Estratégia | Nome curto | Contexto | Motivo da troca (se houve) |
|:-----------|:-----------|:---------|:---------------------------|
|  |  |  |  |

## 5.2 Diário de Tentativas
Uma linha por tentativa relevante, marcando a qual estratégia (S1/S2/S3) ela pertence, obviamente precisa ser em ordem cronológica. A coluna "Quando" precisa bater com a ordem cronológica do evidencias.log, é isso que comprova que o fluxo é real. "Hipótese/Causa" só é preenchida quando o resultado foi falha ou parcial.
| # | Estrat. | O que tentei | Resultado | Hipótese/Causa (se falhou) | Quando | Evidência |
|:--|:--------|:-------------|:----------|:---------------------------|:-------|:----------|
|  |  |  |  |  |  |  |

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
