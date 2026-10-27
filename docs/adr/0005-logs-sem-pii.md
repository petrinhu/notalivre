# ADR 0005: logs sem PII, segredo e inscrição do emitente (ADR-S4)

**Status:** ACEITO por Narciso (CISO) em 25/10/2026 (N3), com as mudanças marcadas `[alterado pelo CISO, 25/10/2026]`. Ver a seção "Revisão do CISO (25/10/2026)" no fim. A relação entre a chave de acesso e o Id da NFS-e é INFERÊNCIA a conferir no manual antes de NL-2.7 (seção "Pendências"); se não se confirmar, a chave é redigida por inteiro, sem mudar este aceite.
**Data:** 25/10/2026.
**Origem:** recomendação do CTO no plano, NL-1.9 (mascarar a inscrição do emitente no Id da DPS e na chave de acesso); spec 4.6 (requisito do brief de logar ID da DPS e chave de acesso, campos em aberto).
**Relacionadas:** ADR-0002 (caminho do A1 fora do log), ADR-0003 (segredos fora do log), `docs/seguranca/modelo-de-ameacas.md` (C7.x).

## Contexto

- FATO, spec 4.6: o brief pede logs detalhados com ID da DPS e chave de acesso. Os campos que entram nos logs estavam em aberto.
- FATO, lido em 25/10/2026, OWASP Logging Cheat Sheet: não gravar diretamente, em log, "Sensitive personal data and some forms of personally identifiable information (PII)", senhas, tokens de acesso, chaves criptográficas, dados de conta e segredos primários. O documento recomenda remover, ofuscar, higienizar, aplicar hash ou cifrar; recomenda "deletion, scrambling or pseudonymization" para dado pessoal; e pede sanitização de todo dado de evento para evitar injeção em log (https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html).
- FATO, `docs/superpowers/plans/2026-10-25-notalivre-plano.md` NL-1.9 e `docs/pesquisa/onda1_leiaute.md` seção 3: o Id da DPS é "DPS" + município (7) + tipo de inscrição (1) + inscrição (14) + série (5) + número (15), totalizando 45 caracteres. Para CPF, a inscrição ocupa as 14 posições com `000` à esquerda. Tipo 1 é CPF, tipo 2 é CNPJ (`docs/pesquisa/onda1_leiaute.md` 1.3 e 3, anexo de 20/10, linha 639 do dump).
- FATO, `docs/pesquisa/onda1_leiaute.md` seção 3: o pattern do XSD (`[0-9]{14}`) aceita zeros à direita e à esquerda. A posição do CPF é regra semântica, e precisa ficar no código e no teste.
- FATO, XSD de Produção Restrita 1.01, `tiposSimples_v1.01.xsd`, linhas 31 a 41 (tipo `TSIdNFSe`, lido em 25/10/2026 na pasta de download da pesquisa, `/var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/`): o Id da NFS-e tem 53 posições, com o padrão `NFS[0-9]{9}[0-9A-Z]{14}[0-9]{27}`. A documentação dá a formação: "NFS" + Cód.Mun. (7) + Amb.Ger. (1) + Tipo de Inscrição Federal (1) + Inscrição Federal (14) + No. NFS-e (13) + AnoMes Emis. (4) + Cód.Num. (9) + DV (1). O mesmo texto está no Anexo I de 20/10, linha 541 do dump.
- FATO, dump do Anexo I de 20/10 (`anexo1_prodrest.txt`, linhas 649 e 652, campos `chNFSeRej` e `chSubstda`): a chave de acesso tem tamanho 50.
- INFERÊNCIA: a chave de 50 posições é o Id de 53 sem o prefixo "NFS" (7 + 1 + 1 + 14 + 13 + 4 + 9 + 1 = 50). A inscrição do emitente, portanto, está na chave, como o CTO supôs. Conferir no manual da API antes de NL-2.7.

## Decisão (proposta)

### 1. Três classes de dado no log

| Classe | O que entra | Exemplos |
|---|---|---|
| **Proibido** | Nunca aparece, nem mascarado | Senha-mestra, senha do certificado, senha SMTP, tokens OAuth, DEK, KEK, código de recuperação, conteúdo do A1, CPF ou CNPJ completo de tomador, nome, endereço, telefone, e-mail, inscrição municipal, corpo de e-mail e anexo. `[alterado pelo CISO, 25/10/2026]` Também: XML da DPS, da NFS-e e de evento; corpo de requisição e de resposta HTTP; texto livre de mensagem de erro da API (só o código `E` entra); cabeçalhos de autenticação e cookies; dados do certificado (nome e CPF ou CNPJ do titular, número de série); nome e caminho de arquivo |
| **Mascarado** | Aparece com a parte sensível substituída | Inscrição do emitente no Id da DPS e na chave de acesso (regra abaixo). O caminho do arquivo A1 não entra em nenhuma classe (ADR-0002) |
| **Permitido** | Aparece como está | Município, tipo de inscrição, série, número da DPS, código de evento, código de erro `E` da API, status HTTP, timestamp, identificador de correlação, versão do app, nome da operação. `[alterado pelo CISO, 25/10/2026]` Também o identificador interno do emissor e do tomador (número local do banco, nunca CPF, CNPJ ou e-mail) |

### 2. Mascaramento da inscrição: todos os tipos, não só CPF

- **Regra:** os 14 caracteres da inscrição são substituídos por `#` (14 `#`), sem nenhum trecho da inscrição. A regra vale para tipo 1 (CPF) e tipo 2 (CNPJ).
- **Id da DPS, forma logada:** `DPS` + município (7) + tipo (1) + `##############` + série (5) + número (15). Posições: 1 a 3, 4 a 10, 11, 12 a 25, 26 a 30, 31 a 45.
- **Contesta a recomendação do CTO** ("mascarar só para emitente CPF"). Razões: (a) o log decide se mascara pelo tipo, e um erro de classificação vaza o CPF; mascarar sempre é fail secure; (b) INFERÊNCIA: o CNPJ de MEI tem como titular uma pessoa física, e a fronteira entre dado da empresa e dado da pessoa é difícil de fixar no log; (c) o custo é pequeno, porque o suporte ainda vê município, tipo, série e número.
- **Chave de acesso, forma logada:** `NFS` + Cód.Mun. (7) + Amb.Ger. (1) + tipo (1) + `##############` + nº da NFS-e (13) + AAMM (4) + Cód.Num. (9) + DV (1), no Id de 53 posições. Na chave de 50 posições (sem o prefixo `NFS`, INFERÊNCIA da seção Contexto), a mesma máscara ocupa as posições 10 a 23. A máscara vale para os dois tipos de inscrição, como no Id da DPS. O requisito do brief (spec 4.6) de logar a chave fica atendido na forma mascarada, e a chave inteira nunca entra no log.

### 3. Mecanismo

- **Allowlist, não denylist:** o `Log` (Foundation) só aceita campos declarados por nome. Campo desconhecido é descartado e contado num contador de descartes, sem o valor.
- **Redação no ponto de escrita:** a máscara e a allowlist ficam no `Log` da Foundation, e não nos pontos de chamada. Assim, um chamador novo não contorna a regra.
- **Sanitização:** quebras de linha e caracteres de controle são trocados antes da escrita (OWASP, injeção de log).
- **Stack trace:** só com tipo da exceção e posição no código. Mensagem de exceção de biblioteca passa pela mesma allowlist.
- `[alterado pelo CISO, 25/10/2026]` **Saída de Qt e de bibliotecas:** o app instala `qInstallMessageHandler` no início do processo, e toda mensagem de `qDebug`, `qWarning`, `qCritical` e `qFatal` (do app, do Qt e do QML) passa pelo `Log` e pela allowlist. Nada vai cru para a saída de erro, que no Flatpak pode acabar no journal do usuário. Erro do OpenSSL e do xmlsec entra só pelo código numérico e pelo nome da função, nunca pelo texto montado com dado de entrada.
- **Erro para o usuário:** não mostra conteúdo de log. Mostra mensagem genérica com o código `E` da API.

### 4. Auditoria (separada do log técnico)

- Registra quem, o quê, quando, de onde (instalação e emissor) e em qual objeto, para: emitir, cancelar, substituir, trocar A1, desbloquear o cofre, exportar ou apagar tomador (LGPD, art. 18), enviar e-mail e alterar cadastro.
- `[alterado pelo CISO, 25/10/2026]` Também registra recuperação pelo código ou pelo A1 e a troca forçada que vem depois (ADR-0002, decisão 7). Objeto, emissor e destinatário entram pelo identificador interno, nunca por CPF, CNPJ ou e-mail; a mesma allowlist do log técnico vale para a auditoria.
- **Append-only:** cada registro traz o hash do anterior (cadeia de hash), o que torna visível a alteração no meio do arquivo. Não é proteção contra quem controla a máquina. Não há servidor de auditoria, porque o app não tem telemetria (spec 3.2).
- Não grava corpo de nota, corpo de e-mail nem segredo.
- Retenção: a definir com Cláudio (LGPD), em aberto.

### 5. Retenção e permissão do log técnico

- Arquivo no diretório de estado do app, com permissão `0600`.
- Rotação por tamanho e por dias, com teto declarado (proposta: 10 MiB por arquivo, 5 arquivos). Valores a confirmar.

## Alternativas consideradas

- **Só mascarar o CPF, como o CTO propôs:** rejeitada, pelos motivos da seção 2.
- **Gravar a chave de acesso inteira no log:** rejeitada até o layout ser confirmado, porque a chave pode carregar a inscrição.
- **Denylist de campos proibidos:** rejeitada. Um campo novo sem nome na lista vaza. A allowlist falha fechada.
- **Hash da inscrição no log:** rejeitada. O CPF tem 10^11 valores, e o hash sem chave é revertido por dicionário.

## Consequências

- O suporte recebe município, série e número, sem a inscrição. Para correlacionar nota e log, usa-se o identificador de correlação e o número da DPS.
- Há custo de implementação no `Log` e nos testes (NL-2.7).

## Verificação

- NL-2.7, varredura obrigatória: roda a suíte ponta a ponta com fixtures que contêm CPF, CNPJ, senha, token e chave. Procura cada valor das fixtures nos logs. Resultado esperado: zero ocorrências, com a contagem de linhas varridas impressa, e zero é sinal de varredura quebrada se o total for 0 (L-36).
- NL-2.7, sabotagem de estreia: logar a senha de propósito e ver o portão ficar vermelho (plano, NL-2.7).
- NL-2.7, teste posicional: o Id da DPS gerado com o CPF da fixture tem `##############` nas posições 12 a 25. O teste procura o CPF da fixture no log, e não um padrão de regex, porque o XSD não distingue zero à esquerda de zero à direita (`docs/pesquisa/onda1_leiaute.md` 3).
- NL-2.7, teste posicional da chave: a chave de acesso gerada com a inscrição da fixture tem `##############` nas posições 13 a 26 do Id de 53 (10 a 23 da chave de 50), e a inscrição da fixture não aparece no log.
- NL-2.7, teste de allowlist: campo novo sem declaração é descartado e contado.
- NL-2.7, teste de injeção: entrada com quebra de linha não cria linha falsa no log.
- `[alterado pelo CISO, 25/10/2026]` NL-2.7, teste do manipulador de mensagens: um `qWarning` com o CPF da fixture não aparece nem no arquivo de log nem na saída de erro capturada do processo. Sabotagem: não instalar o manipulador e ver o teste vermelho.
- `[alterado pelo CISO, 25/10/2026]` NL-2.7, teste de erro da API: uma resposta do servidor falso com texto livre contendo o CPF da fixture gera só o código `E` no log.

## Pendências

- **Relação chave de acesso e Id da NFS-e:** INFERÊNCIA de que a chave de 50 posições é o Id de 53 sem o prefixo `NFS`. Conferir no manual da API antes de NL-2.7. Se a relação não se confirmar, a chave volta a ser redigida por inteiro (seção 2), sem nova decisão do líder.
- **Retenção da auditoria** e **teto de rotação**: com Cláudio.

## Riscos residuais

- Exceção de biblioteca de terceiro pode trazer dado pessoal em texto livre. Mitigado pela allowlist e pela varredura de NL-2.7. Residual baixo.
- O log fica no computador do usuário, sem proteção contra quem tem a máquina. A proteção do log é o sistema de arquivos e a sandbox.

## Fontes

- OWASP Logging Cheat Sheet, lido em 25/10/2026: https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html
- XSD `tiposSimples_v1.01.xsd`, linhas 31 a 41, lido em 25/10/2026 (`/var/tmp/notalivre-pesquisa-onda1/xsd-dl/x/`).
- Pesquisa, não relida nesta sessão: `docs/pesquisa/onda1_leiaute.md`, seções 1.3 e 3; dump `anexo1_prodrest.txt` linhas 541, 649 e 652 (no scratchpad da sessão).
- Plano, NL-1.9, NL-2.7 e N3.
- Spec, seção 4.6.

## Revisão do CISO (25/10/2026)

**Revisor:** Narciso (CISO), distinto do security-engineer que escreveu este ADR (L-12). Modo autônomo (L-15).

**Veredito: ACEITO** (N3). As mudanças que exigi já estão no corpo, marcadas `[alterado pelo CISO, 25/10/2026]`. O que resta é verificação em NL-2.7 e a conferência da chave de acesso no manual, que já tem saída fechada (chave redigida por inteiro se a relação não se confirmar).

**Decisões:**

1. **Máscara da inscrição sempre, para CPF e para CNPJ** (ponto 4 do autor). Contesto a recomendação do CTO de mascarar só o CPF, pelas razões da seção 2: decidir pelo tipo é um ponto de falha que vaza CPF em silêncio, e o CNPJ de MEI tem pessoa física como titular. O suporte não perde o que precisa (município, tipo, série, número e o identificador de correlação).
2. **Lista de proibidos ampliada:** XML, corpos HTTP, texto livre da API, cabeçalhos de autenticação, dados do certificado e nomes de arquivo. Cada um carrega PII ou segredo por construção.
3. **Toda saída do Qt e das bibliotecas passa pelo `Log`** (manipulador de mensagens), com teste e sabotagem.
4. **Auditoria só com identificador interno** e com registro da recuperação.

**Por quê:** a allowlist só protege o que passa por ela. O vazamento que sobra num desktop vem do que não passa: saída de erro do Qt e das bibliotecas, texto de erro devolvido pela API com dado do tomador, e XML inteiro em mensagem de depuração. As mudanças põem esses caminhos sob a mesma regra e dão a cada um um teste que tem que ficar vermelho por sabotagem (L-36).

**Nada deste ADR sobe ao líder.** Retenção da auditoria e teto de rotação seguem com Cláudio, como o ADR já diz.
