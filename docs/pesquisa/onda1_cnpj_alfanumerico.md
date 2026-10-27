# Pesquisa Onda 1: DV do CNPJ alfanumérico

**Data:** 25/10/2026
**Autor:** backend-engineer
**Item:** NL-1.1, pesquisa de abertura da Onda 1 (L-22: pesquisa antes do planejamento)

**Pergunta (a):** qual é a regra oficial do dígito verificador (DV) do CNPJ alfanumérico (vigente a partir de julho de 2026) e quais vetores de teste oficiais existem?

**Legenda:** FATO = afirmado por fonte com URL ou verificado por script. INFERÊNCIA = conclusão minha, marcada assim. NÃO ENCONTRADO = a fonte consultada não traz a informação. BLOQUEADO = a fonte não pôde ser lida (L-57, sem contornar proteção).

**Fontes oficiais usadas:**

- [F1] Serpro, "Cálculo dos dígitos verificadores de CNPJ alfanumérico" (PDF, 4 páginas): https://serpro.gov.br/menu/noticias/videos/calculodvcnpjalfanaumerico.pdf
- [F2] Receita Federal, "Novo CNPJ Alfanumérico, Perguntas & Respostas" (PDF, 14 páginas, lido página a página por imagem renderizada, pois o texto não é extraível): https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf

Citações "Q.n" referem-se às perguntas numeradas de [F2].

---

## a1. Fonte normativa e documento técnico

**FATO.** A norma é a Instrução Normativa RFB nº 2.229/2024, que altera a IN RFB nº 2.119/2022 e acrescenta o formato alfanumérico do CNPJ, com implantação prevista a partir de julho de 2026. Fonte: [F2] Q.26 ("Instrução Normativa 2.119/2022, com as alterações introduzidas pela Instrução Normativa RFB 2.229/2024, o cronograma para implementação do CNPJ alfanumérico será julho de 2026").

**FATO.** Documentos técnicos oficiais que descrevem o cálculo:
- [F1] Serpro, cálculo passo a passo (1.1 primeiro DV, 1.2 segundo DV), com exemplo completo.
- [F2] Receita Federal, Q.14 (regra do módulo 11, tabela ASCII) e exemplo "Exemplo de Cálculo de Dígito Verificador de um CNPJ alfanumérico" logo após a Q.14.

**FATO, data da IN: de fonte secundária, não confirmada no texto oficial.** A data "15/10/2024" e a publicação no DOU em "16/10/2024" aparecem apenas em fontes secundárias: lex.com.br (título da IN) e okai.com.br (título e URL com 2024-10-16). Ambas são espelhos não oficiais.

**BLOQUEADO.** Tentei ler o texto da IN no mirror okai.com.br; o servidor respondeu HTTP 403. Não há como ler o texto da IN com as fontes acessíveis. Não tentei contornar o bloqueio (L-57).

**NÃO ENCONTRADO.** Anexo XV da IN (o texto normativo do formato e do DV). O lex.com.br marca o Anexo Único como exclusivo para assinantes. O texto normativo do DV, portanto, foi lido só pelo documento técnico [F1] e pela Q&A [F2], não pelo anexo.

**NÃO CONFIRMADO.** As datas de entrada em produção que aparecem em blogs ("27/07/2026" e "31/07/2026") não foram encontradas em fonte oficial. Não as uso neste documento.

**Observação sobre [F1].** Os metadados do PDF indicam criação em 28/07/2022 (ferramenta "Microsoft: Print To PDF"). Essa data é anterior à IN 2.229/2024, então não a uso para datar o documento nem para afirmar que a regra é anterior à norma. A data real de [F1] não está no documento.

---

## a2. Algoritmo exato

**FATO, conversão de caractere.** Cada caractere vira `valor = código ASCII - 48`. Para dígitos, o valor é o próprio dígito (0 a 9). Para letras, A = 65 - 48 = 17, B = 18, ..., Z = 90 - 48 = 42. Fontes: [F2] Q.14 (tabela ASCII, de "0"=48 a "Z"=90) e [F1] 1.1 ("atribuir o valor da coluna 'Valor para cálculo do DV', ou subtrair 48 do 'Valor ASCII'").

**FATO, pesos.** Pesos de 2 a 9 aplicados da direita para a esquerda, recomeçando após o oitavo caractere. Para o 1º DV (12 caracteres): `[5,4,3,2,9,8,7,6,5,4,3,2]`. Para o 2º DV (13 caracteres, com o 1º DV no fim): `[6,5,4,3,2,9,8,7,6,5,4,3,2]`. Fonte: [F1] 1.1 e 1.2, tabelas de pesos.

**FATO, módulo e resto.** Soma dos produtos valor × peso, depois `resto = soma mod 11`. Se o resto for 0 ou 1, o DV é 0. Senão, o DV é `11 - resto`. Fonte: [F1] 1.1 ("Se o resto da divisão for igual a 1 ou 0, o primeiro dígito será igual a 0 (zero). Senão, o primeiro dígito será igual ao resultado de 11 - resto."). A Q.14 de [F2] não descreve o tratamento de resto 0 e 1; essa regra vem só de [F1].

**FATO, sequência.** 1º DV sobre os 12 primeiros caracteres. 2º DV sobre os 12 primeiros caracteres seguidos do 1º DV (13 caracteres). Fonte: [F1] 1.2.

**FATO, fórmula igual ao CNPJ numérico.** [F2] Q.14: "O Dígito Verificador será calculado pelo algoritmo do módulo 11". A conversão ASCII - 48 mapeia cada dígito para si mesmo, então a fórmula acima coincide com o cálculo do CNPJ numérico. Essa coincidência é INFERÊNCIA minha (derivação dos mesmos pesos e do mesmo módulo), confirmada pelo vetor numérico do item a5.

**Script de conferência:** `/var/tmp/notalivre-pesquisa-onda1/a/cnpj_dv.py`, saída completa colada em "Saída do script" abaixo.

---

## a3. Letras permitidas, posições, DV e caixa

**FATO, posições.** Posições 1 a 12 podem ser letras ou dígitos. Raiz = posições 1 a 8; ordem do estabelecimento = posições 9 a 12. Posições 13 e 14 (DV) são sempre numéricas. Fontes: [F2] Q.2 (14 posições; DV é o dígito verificador, calculado por módulo 11), Q.23 ("CNPJ Raiz (8 primeiras posições)"; "número de ordem da filial (9ª a 12ª posições) poderá ser numérico ou alfanumérico"); [F1] 1 ("doze caracteres alfanuméricos e dois dígitos verificadores numéricos").

**FATO, letras.** As 26 letras de A a Z. Fonte: [F2] Q.1 ("26 letras de A até Z") e Q.2.

**FATO, sem exclusão oficial de I, O, Q, F.** [F2] Q.45 orienta que "as adaptações nos sistemas considerem todas as letras do alfabeto brasileiro". Nenhuma pergunta da Q&A oficial exclui I, O, Q ou F. Uma fonte secundária citada numa busca (sem URL verificada por mim) afirmava que a Receita recomendaria evitar essas letras. Essa afirmação contradiz [F2] Q.45 e não a adoto.

**FATO, combinações proibidas.** [F2] Q.38 e Q.45: "algumas combinações de letras serão proibidas durante a geração do identificador" (palavras ofensivas ou que gerem confusão com números), e esse controle "será feito internamente pela própria Receita Federal".

**NÃO ENCONTRADO, lista de combinações proibidas.** A Q&A não publica a lista. Consequência para o produto: a validação local só consegue checar formato (14 caracteres, posições e alfabeto) e DV. Não consegue rejeitar combinação proibida. Isso precisa constar no escopo do NL-1.1 como limitação conhecida.

**FATO, caixa.** [F2] Q.2 define as letras como "letras maiúsculas de A até Z".

**NÃO ENCONTRADO, minúscula.** Nem [F1] nem [F2] dizem o que fazer com minúscula.

**INFERÊNCIA, risco de minúscula.** `ord('a') - 48 = 49`, que não é o valor de `A` (17). Se a entrada aceitar minúscula sem normalizar, o DV sai errado sem erro visível. Para o produto, há duas opções: normalizar para maiúscula na borda ou rejeitar com erro de formato. A escolha é decisão de produto e fica para o líder (L-01).

---

## a4. Vetores de teste oficiais

Valores do script (saída colada abaixo). Todos os vetores são exemplos publicados pela Receita ou pelo Serpro. Não são CNPJs de empresa identificada.

| # | Vetor (14 caracteres, sem máscara) | Máscara na fonte | DV esperado | DV pelo script | Resultado | Origem |
|---|---|---|---|---|---|---|
| V1 | `12ABC34501DE` + `35` | 12.ABC.345/01DE-35 | 35 | 35 | OK | [F1] p.3-4 e [F2] Q.14 (exemplo) |
| V2 | `AA345678000A` + `29` | AA345678/000A-29 | 29 | 29 | OK | [F2] Q.23 Ex2 |
| V3 | `12345678000A` + `08` | 12.345.678/000A-08 | 08 | 08 | OK | [F2] Q.23 Ex3 |
| V4 | `123456780001` + `95` (numérico) | 12.345.678/0001-95 | 95 | 95 | OK | [F2] Q.17 |
| X1 | `AA3456780003` + `29` | AA345678/0003-29 | 29 | **86** | DIVERGE | [F2] Q.23 Ex1 |

**FATO, V1 é o vetor de referência.** Serpro e Receita publicam o mesmo exemplo completo, com resultado final 12.ABC.345/01DE-35. Passos: soma 459, resto 8, DV1 = 3; soma 424, resto 6, DV2 = 5 ([F1] 1.1 e 1.2; [F2] Q.14).

**FATO, V3 é o único vetor oficial que exercita resto 0 ou 1.** Passos: soma 254, resto 1, DV1 = 0; soma 267, resto 3, DV2 = 8. Isso cobre o ramo "resto 0 ou 1, DV = 0", que nenhum outro vetor cobre.

**FATO, X1 diverge.** [F2] Q.23 Ex1 publica `AA345678/0003-29`. O script calcula `86`: soma 366, resto 3, DV1 = 8; soma 412, resto 5, DV2 = 6.

**Argumento de que X1 é inconsistente com a própria regra (INFERÊNCIA, aritmética conferida pelo script).** Ex1 e Ex2 diferem só na posição 12 (`3` contra `A`, valor 17). A posição 12 tem peso 2 no primeiro cálculo, então a soma muda em `(17 - 3) × 2 = 28`, e `28 mod 11 = 6`. Os restos das duas somas diferem em 6 (mod 11). Pela regra de [F1], o DV1 depende só do resto (0 e 1 viram 0; os demais viram `11 - resto`), e restos que diferem em 6 mod 11 nunca produzem o mesmo DV1. Como Ex1 e Ex2 ambos trazem DV1 = 2, pelo menos um dos dois está errado. Ex2 bate com o cálculo, então o erro está em Ex1.

**INFERÊNCIA, causa provável.** O `29` de Ex1 parece copiado de Ex2. O DV que a regra dá a `AA345678/0003` é `86`. Não tenho outra fonte oficial que confirme isso.

**Decisão para o projeto.** Não usar X1 como vetor de teste. Como validação externa, a Receita oferece o Simulador Nacional de CNPJ (ver a6). Eu não o acionei: é aplicação JavaScript em navegador, e o L-57 proíbe automação de browser sem autorização. Conferência manual pelo líder ou pelo time, se quiserem confirmar X1.

**FATO, Q.23 imprime os exemplos sem máscara.** Q.23 escreve `AA345678/0003-29`, sem pontos, enquanto Q.21 define a máscara com pontos. Não é erro de cálculo, mas o teste deve usar a entrada sem máscara e comparar a saída com a máscara da a6.

**NÃO ENCONTRADO, vetor oficial adicional.** Não achei outro exemplo oficial além dos cinco da tabela nas fontes que consegui ler. Não usei o `12.ABC.345/01DE-35` como CNPJ de empresa real: é exemplo ilustrativo da Receita e do Serpro.

---

## Saída do script

Comando: `python3 -I /var/tmp/notalivre-pesquisa-onda1/a/cnpj_dv.py` (saída gravada em `/var/tmp/notalivre-pesquisa-onda1/a/saida_cnpj_dv.txt`; exit status 1 porque X1 diverge).

```
12ABC34501DE | 1o: soma=459 resto=8 DV1=3 | 2o: soma=424 resto=6 DV2=5 | esperado=35 calculado=35 | OK | Serpro PDF p.3-4; RFB Q&A Q14 (exemplo 12.ABC.345/01DE-35)
AA3456780003 | 1o: soma=366 resto=3 DV1=8 | 2o: soma=412 resto=5 DV2=6 | esperado=29 calculado=86 | DIVERGE | RFB Q&A Q23 Ex1 (AA345678/0003-29)
AA345678000A | 1o: soma=394 resto=9 DV1=2 | 2o: soma=442 resto=2 DV2=9 | esperado=29 calculado=29 | OK | RFB Q&A Q23 Ex2 (AA345678/000A-29)
12345678000A | 1o: soma=254 resto=1 DV1=0 | 2o: soma=267 resto=3 DV2=8 | esperado=08 calculado=08 | OK | RFB Q&A Q23 Ex3 (12.345.678/000A-08); exercita resto 0/1
123456780001 | 1o: soma=222 resto=2 DV1=9 | 2o: soma=237 resto=6 DV2=5 | esperado=95 calculado=95 | OK | RFB Q&A Q17 (12.345.678/0001-95), numerico
vetores=5 divergencias=1
```

**Leitura do resultado.** Quatro de cinco vetores batem, incluindo o de referência V1 e o de resto 0 ou 1 (V3). A divergência está só em X1, com a análise de a4. O algoritmo confere com as fontes. O PARE da tarefa valia para o exemplo de referência, e V1 bateu.

---

## a5. CNPJ numérico antigo

**FATO, continua válido com o mesmo DV.** [F2] Q.15: "Para quem já tem o número do CNPJ nada muda. Os atuais números permanecerão válidos assim como os seus dígitos verificadores." Também Q.29 (quem já tem CNPJ não precisa de alteração) e Q.36 ("Nenhum CNPJ será cancelado ou anulado").

**FATO, coexistência.** [F2] Q.7: "tanto o formato numérico quanto o alfanumérico será válido em todos os processos". Q.27 e Q.28: após julho de 2026 ainda podem ser atribuídos CNPJs totalmente numéricos a novas inscrições, de forma aleatória. Logo, o validador precisa aceitar os dois formatos.

**FATO, mesmo módulo.** [F2] Q.14: "algoritmo do módulo 11", sem distinção de formato.

**INFERÊNCIA, numérico é caso particular.** Com `valor = ord(c) - 48`, cada dígito vale ele mesmo. A fórmula da a2 então reduz ao cálculo do CNPJ numérico (mesmos pesos, mesmo módulo, mesmo tratamento de resto). Essa redução é derivação minha, não texto da fonte.

**Conferência.** Vetor V4, `12.345.678/0001-95`, sai com DV `95` pelo script (ver saída). Esse exemplo vem da Q.17 de [F2]. É exemplo ilustrativo, não CNPJ de empresa identificada. Não consultei o cadastro da Receita para confirmar que o número existe. A validade do algoritmo não depende disso.

---

## a6. Máscara de exibição

**FATO, mesma máscara do numérico.** [F2] Q.21: "A máscara dos CNPJ's alfanuméricos será exatamente igual ao do CNPJ numérico, ou seja, ambos terão o mesmo formato e conterão os mesmos 14 caracteres hoje utilizados ('XX.XXX.XXX/XXXX- DV')." A máscara é, portanto, `XX.XXX.XXX/XXXX-DD`, com pontos, barra e hífen.

**FATO, caixa na exibição.** As letras são maiúsculas ([F2] Q.2). Exemplo da Serpro: `12.ABC.345/01DE-35` ([F1] 1.2, "Resultado final").

**FATO, zeros à esquerda.** [F2] Q.21: todos os CNPJs têm 14 caracteres, podendo ter zeros à esquerda.

**FATO, divergência de apresentação.** Q.23 escreve os exemplos sem máscara (`AA345678/0003-29`). A máscara oficial é a da Q.21. Não é erro de cálculo.

**FATO, validação oficial.** [F2] Q.47 a Q.49: a Receita disponibiliza o Simulador Nacional de CNPJ, gratuito, que valida "se um CNPJ atende às regras de formação e dígitos verificadores". Endereço, segundo Q.48: https://servicos.receitafederal.gov.br/servico/cnpj-alfa/simular . Página oficial do projeto, também citada em Q.48: https://www.gov.br/receitafederal/pt-br/acesso-a-informacao/acoes-e-programas/programas-e-atividades/cnpj-alfanumerico . Nenhuma das duas foi acessada por mim (o simulador é aplicação de navegador, L-57).

**NÃO ENCONTRADO, regra de remoção de máscara.** As fontes não dizem quais separadores são aceitos na entrada. Decisão de produto, a tratar no NL-1.1.

---

## Impacto em NL-1.1

- `cnpjAlfanumericoValidoAceito`: usar V1, entrada `12ABC34501DE` com DV `35`. Comentário: `// Serpro, Cálculo dos DV de CNPJ alfanumérico, 1.1-1.2: https://serpro.gov.br/menu/noticias/videos/calculodvcnpjalfanaumerico.pdf; RFB Q&A Q.14`.
- Teste de resto 0 ou 1 (DV1 = 0): usar V3, `12345678000A` com DV `08` (máscara `12.345.678/000A-08`). Comentário: `// RFB Q&A Q.23 Ex3: https://www.gov.br/receitafederal/pt-br/centrais-de-conteudo/publicacoes/perguntas-e-respostas/cnpj/cnpj-alfanumerico.pdf`.
- Letra na ordem e compatibilidade numérica: V2 (`AA345678000A`, DV `29`) e V4 (`123456780001`, DV `95`, numérico, Q.17 da mesma URL).
- Não usar `AA345678/0003-29` (X1): divergente da regra, ver a4.
- Decisões pendentes para o líder: minúscula (normalizar ou rejeitar, a3) e entrada com máscara. Limitação a registrar no escopo: combinações proibidas não são validáveis localmente (a3).

---
