# Auditoria independente de fechamento da Onda 2 (NotaLivre)

**Auditor:** internal-auditor, independente (não implementou a Onda 2; não escreveu no repositório nem fez commit).
**Passadas:** passada 1 (30/10/2026, `auditoria-onda2-passada1.md`), passada 2 (31/10/2026, `auditoria-onda2-passada2.md`), passada 3 (01/11/2026, `auditoria-onda2-passada3.md`), passada 4 (03/11/2026, `auditoria-onda2-passada4.md`: parte A no alvo `2c10320`; parte B no alvo `f8242c7` com parada antes do passo 9; retomada pela Emenda 2 no alvo `df229cc`; atualização final com os IMPORTANTES fechados).
**Branch:** `onda-2` (`git ls-remote origin refs/heads/onda-2` conferido em cada alvo).
**Alvos:** parte A `2c103208f9dea6484634eaa9b2e546c55ad48521`; parte B `f8242c71cfc027adf0e4fa90fc8b8bd1b5b2b73c`; alvo final `df229cce20f85e23b17314f5101522fa1abba78b` (merge da FECH-2-7, com `90a9f68` e `55f9cfc`).
**Plano e definição:** `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, "Definição de fechamento da Onda 2" (itens 1 a 12) e a emenda do líder ao item 9 (Q1, "Não exercida (Recomendado)"), logo depois da definição. Linhas conferidas no blob do alvo final pelo texto, não pelo número.
**Evidências:** CI lido de dentro do runner com `gh run view --log` (L-49): run `41577685449` (alvo `2c10320`), run `41656719385` (alvo `f8242c7`), run `41675477140` (alvo `df229cc`, `headSha` conferido, 10 jobs `success`). Nos nativos fedora44, mint22.3 e ubuntu26.04: `100% tests passed, 0 tests failed out of 434`; manjaro: `100% tests passed out of 434`; flatpak: `100% tests passed out of 340`. Lista do Flatpak no log do run: `fora=95 listados=95 divergencias=0` nos quatro jobs. Revisões e registros em `/var/tmp/notalivre-logs/`.
**Frase-guarda (L-21), verbatim:** "Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição."
**Dado pessoal:** este capítulo não traz CNPJ, CPF nem inscrição municipal do emitente. A chave de acesso aparece só na forma mascarada do documento versionado.

---

## 1. Veredito

**A Onda 2 FECHA, com ressalvas registradas.** As 12 condições da definição estão ATENDIDAS. Não há CRÍTICO aberto. Não há IMPORTANTE aberto: os três IMPORTANTES de merges (N-01, N-11, N-14) estão aceitos residualmente com controle e revisão retroativa citados, e os dois IMPORTANTES da revisão retroativa do N-01 (N-19 e N-20) foram fechados pela NL-2.15, com revisões APROVA cobrindo o SHA final. Todas as linhas da W2 estão em ✅, e nenhuma em 🔍.

Ressalvas que não bloqueiam e ficam registradas:
- **N-24 (desvio de processo).** O registro (linha 595) declara que o CTO leu `saida-tentativa-1.txt` da pasta de devolução para medir o DV das posições 10 a 23 da chave. A regra de não leitura da devolução consta do briefing deste auditor. O main refez a conferência e reporta DV válido, sem imprimir valores. Este auditor não reproduz o número. Fica para ciência do líder.
- **N-21 (aceito residual, DA-2.125).** O caminho `bytesEscritos > 0` tem teste real; o corte do temporizador depois do corpo escrito não tem teste, e o destino é uma linha na INBOX para a NL-3.7.
- Itens COSMÉTICOS abertos (seção 3.3).

**Pergunta ao líder (AskUserQuestion, pelo main):** ciência do desvio N-24, e ratificação da DA-2.125 (a ratificar).

---

## 2. Veredito item a item

| # | Condição da definição | Veredito | Evidência | Achado ligado |
|---|---|---|---|---|
| 1 | NL-2.0: todo verde local com SHA no topo, sem teste de outra fatia; `xmlsec1` nas 4 imagens | ATENDIDA (pela DA-2.118) | Dos 410 verdes de suíte da Onda 2 (nome `verde` com `% tests passed`), 407 têm SHA na linha 1. O único sem SHA, `NL-2.10a-ci-root-verde-novo-dfc93db.log` (linha 1 `uid=0`, SHA só no nome), foi substituído pelos verdes por SHA de `165f4ff` nas 4 imagens (`NL-2.10j-J1-verde-r2final-165f4ff-{fedora44,manjaro,mint22.3,ubuntu26.04}.log`: linha 1 `SHA: 165f4ffc335180779f465b259bda5a6c63c45485`, `NL-RC=0`, `100% tests passed`). `dfc93db` é ancestral de `165f4ff` (rc=0). Substituição registrada (DA-2.118), pelo precedente da DA-CISO de 31/10/2026 11:21:36, ratificado pelo líder em 31/10 23:08. O log não foi reescrito. `xmlsec1` rc=0 nas 4 imagens (`NL-2.0-xmlsec1-*.txt`, 27/10/2026). | N-12 |
| 2 | CLI emite DPS que o servidor falso aceita, `xmllint` e `xmlsec1 --verify` | ATENDIDA | Run B2, slot `e2e.cliEmitir.cliEmiteDpsAceitaPeloServidorFalso`: Passed nas 4 nativas | - |
| 3 | Sem certificado de cliente, handshake recusado | ATENDIDA | Run B2: `transporte.semCertificadoDeClienteHandshakeRecusado` e `servidor_falso.semCertificadoRecusado` Passed nas 4 nativas. Mutante `CERT_OPTIONAL` MORTO (rc=8) só no ubuntu26.04, por desenho (DA-2.60) | - |
| 4 | XML adulterado rejeitado | ATENDIDA | Run B2: `xmldsig.xmlAlteradoDepoisDeAssinarReprova` e `servidor_falso.dpsAdulteradaRejeitada` Passed 4/4 | - |
| 5 | Corte depois do aceite: GET /dps/{id} antes de reenvio; 409 nunca reenvia | ATENDIDA | Run B2: `corpoEscritoEConexaoFechadaConsultaAntesDeResponder` (`tests/unit/mid/sefin/cliente/tst_cliente_sefin.cpp:988-1001`, sequência `GET:200, GET:200` e um POST) Passed 4/4; família `conflito409*` 16 Passed | N-21 (residual) |
| 6 | Portão de PII em log, zero ocorrências, visto vermelho por sabotagem | ATENDIDA (ressalva) | Run B2: `cli.portaoPiiVarreLogsDaCli` Passed 4/4. A prova de vermelho vem da passada 3 (`auditoria-onda2-passada3.md:35`), não reconferida nesta passada | - |
| 7 | `.pfx` legado carrega sem desligar o provider default | ATENDIDA | Run B2: `pkcs12.legadoNaoDesligaODefault` Passed 4/4 | - |
| 8 | NL-2.10 executada pelo líder: NFS-e em Produção Restrita, chave de 50 posições registrada | ATENDIDA | `docs/pesquisa/onda2_producao_restrita.md`, seção 1: DPS 9, POST /nfse com status 201, evento `emissao.aceita`, chave de 50 posições na forma mascarada. Origem: fatos da devolução (`FECH-2-0`) | N-24 (origem da medição de DV) |
| 9 | Documento com cada INFERÊNCIA marcada CONFIRMADA, REFUTADA ou NÃO EXERCIDA (emenda Q1), e canon atualizado | ATENDIDA | 21 INF-IDs no documento; 14 dígitos seguidos = 0. Plano com marcas nas linhas 450 a 648 e spec com INF-03 e INF-21. A correção N-13 entrou em `90a9f68` e `df229cc` (FECH-2-7): INF-05 CONFIRMADA (extração do OID e tipo 2) e NÃO EXERCIDA (comparação A1 x DPS), destino NL-4.4; INF-15 com a medição de DV declarada pela DA-2.119 | N-13 (fechado), N-24 |
| 10 | internal-auditor sem CRÍTICO | ATENDIDA | CRÍTICO aberto: 0 | - |
| 11 | CI verde nas quatro distros, como uma linha entre as outras | ATENDIDA | Run `41675477140`: `headSha` = `df229cce20f85e23b17314f5101522fa1abba78b`, `conclusion` success, 10 jobs success; nativos com 434 testes e 0 falhas (fedora44, mint22.3, ubuntu26.04) e manjaro 434 | - |
| 12 | DA-2.1 em diante no registro, para a ratificação do líder | ATENDIDA (ressalva) | 124 IDs `DA-2.N` no registro, de 1 a 124. DA-2.119 a 2.123 existem como resumo na linha 595, sem entrada própria (N-23). Ratificação do líder: DA-2.68 a 2.112 (Q2, 03/11/2026 10:53, "Ratificar todas (Recomendado)"); DA-2.113 a 2.124 (Q3, 03/11/2026 13:30, "Ratificar todas (Recomendado)") | N-16, N-23 |

Resultado: **12 de 12 condições ATENDIDAS**, com ressalvas.

---

## 3. Achados

Estado final somando as passadas 1 a 4 e a atualização final. Severidades pelo manual `AUDITORIAS` (CRÍTICO bloqueia; IMPORTANTE corrigir antes do próximo marco; COSMÉTICO melhoria incremental). A severidade "BAIXA" do revisor R2 foi mapeada para COSMÉTICO.

### 3.1 IMPORTANTES aceitos residualmente (com controle e verificação)

| ID | Passada | Descrição | Evidência | Estado final e controle |
|---|---|---|---|---|
| N-01 | 2, 3, 4 | CI vermelho em merges depois de `1384887`: `3298206`, `a59e1a5`, `dbd6e40`, `c97fa67` (failure), `6dc9472` (cancelled); e nas passadas anteriores, `b34e3b3` (NL-2.6), `062de86` (NL-2.8), `dfd00cb` e `2300d1d` (NL-2.9), três merges do NL-2.11, dois do NL-2.15, `20eb6bf` e `678f93f` (cancelled) | Runs listados em `auditoria-onda2-passada4.md` (passo 5) e no registro | ACEITO RESIDUAL (DA-2.113, ratificada na Q3). Decisão do líder de 01/11 07:29: "Adotar na Onda 3 (Recomendado)". Controle: o critério de fechamento usa só o SHA final, e a regra da Onda 3 entra no `CLAUDE.md` antes da abertura da Onda 3. Verificação: `grep` no canon na abertura da Onda 3 (L-77); auditor da Onda 3 |
| N-11 | 4 | Merges com conteúdo sem revisão registrada antes do merge (8 merges, DA-2.114). `d3d948f` (`docs/pesquisa/parametros_municipais.md`) chegou ao SHA B sem revisão que o cobrisse | Emenda 1, item 1, da parte B | ACEITO RESIDUAL (DA-2.114, Q3), com revisão retroativa: `d3d948f` por FECH-2-R1 (qa-engineer, APROVA na rodada 2 no blob `55f9cfc`; M-1 e M-2 resolvidos; BAIXOS na INBOX). `c97fa67` por encadeamento de revisão (DA-2.123, N-18). Controle: a Onda 3 só aceita merge com APROVA no registro, com hora, citando o SHA igual ao `<merge>^2`. Verificação: auditor da Onda 3 com `git rev-parse <merge>^2` |
| N-14 | 4 | NL-2.5 (`fcd9a3a`, `^2` = `fbc7b94`): 14 arquivos de transporte HTTPS e servidor falso (639 inserções) sem APROVA do SHA final; a última APROVA era de `397b6d5` | `git diff --stat 397b6d5 fbc7b94`; registro sem aprovação da NL-2.5 após 29/10 17:00 (linha 120) | ACEITO RESIDUAL (DA-2.121, Q3), com revisão retroativa FECH-2-R2 (security-engineer, APROVA). m1 (`classificacao_erro.cpp:16`) MORTO, rc=8, por `transporte.classificacao.tabelaDeCodigosDeRede` (`FECH-2-R2-m1-clone-fedora44.log`, SHA `42cdc7f`, origem=archive); m2 (`raizes_producao.cpp:56`) MORTO, rc=8, por `mid.cofre.manifestoComHashDiferenteDoEmbutidoRecusado` (`FECH-2-R2-m2-clone-fedora44.log`, SHA `5bf6650`). Mutantes em clones descartáveis, sem push. Desvio do revisor: commit do clone de m1 com `core.hooksPath` desligado (contrário à L-26, sem efeito no repositório) |

### 3.2 IMPORTANTES fechados nesta atualização

| ID | Passada | Descrição | Evidência de fechamento | Estado final |
|---|---|---|---|---|
| N-19 | 4 (revisão retroativa do N-01, `N-01-revisao-retroativa.md:52-56`) | `651b792`: slots do cliente Sefin fora do Flatpak sem portão que os veja | Portão exato `tools/portoes/fora_do_flatpak.sh` e lista `flatpak/testes_fora_do_flatpak.txt` (presentes no blob `df229cc`); log do run B2 `fora=95 listados=95 divergencias=0`. Revisão `NL-2.15-rev-qa.md` (APROVA COM RESSALVAS; SHA `0828bd1` = `^2` de `c804b95`, linha 5): mutantes M1 a M4 MORTOS (lista sem teste, lista com teste inexistente, teste listado registrado no Flatpak, guarda desligada), rc 1 cada (linhas 14 a 17). Revisão `NL-2.15-rev-devops.md` (APROVA, linha 3) | FECHADO |
| N-20 | 4 (revisão retroativa do N-01, `N-01-revisao-retroativa.md:52-56`) | `018957b`: `apt-get install` no contêiner da CI fora de `tools/conteiner/instalar/` | `apt-get` retirado do job de contêiner. O único `apt-get` fora de `tools/conteiner/instalar/` no blob `df229cc` é `.github/workflows/ci.yml:442`, no job de smoke do host, com o comentário "Plano B da NL-0.5 ... (NL-2.15, DA-2.65, emenda 2)". Revisão `NL-2.15-rev-devops.md` (APROVA, linha 3, seção de `ci.yml` e `apt-get`) | FECHADO |

### 3.3 COSMÉTICOS

| ID | Passada | Descrição | Estado final |
|---|---|---|---|
| N-21 | 4 (revisão R2) | Caminho `bytesEscritos > 0` depois de POST sem teste para o corte do temporizador. O teste `corpoEscritoEConexaoFechadaConsultaAntesDeResponder` (`tst_cliente_sefin.cpp:988-1001`, blob `df229cc`) existe e passa 4/4 no run B2 (conferido por este auditor). Sobra sem teste o corte do temporizador depois do corpo escrito (INFERÊNCIA do revisor, não medida) | ACEITO RESIDUAL (DA-2.125, CTO, a ratificar): rebaixado a COSMÉTICO. Controle: reconciliação por E0014 em qualquer tentativa, coberta por `e0014NaPrimeiraTentativaReconcilia`. Destino: linha da INBOX para a NL-3.7, a ser escrita no commit da FECH-2-6 (pendente) |
| N-24 | 4 (triagem p4b, DA-2.119; registro, linha 595) | Leitura da pasta de devolução por agente (CTO), para medir o DV das posições 10 a 23 da chave. O valor não foi gravado. A ordem deste auditor também proíbe essa leitura | Classificado como DESVIO DE PROCESSO, não bloqueio (decisão do main). Aberto para ciência do líder. O main reporta a conferência do DV (CNPJ e chave válidos); este auditor não reproduz o número |
| N-13 | 4 | INF-05 e INF-15 marcadas CONFIRMADA com a comparação A1 x DPS e a inscrição real não medidas | FECHADO em `df229cc` (FECH-2-7). Ressalva: a medição de DV é a do N-24 |
| N-12 | 4 | Verde de suíte sem SHA na linha 1 (`NL-2.10a-ci-root-verde-novo-dfc93db.log`) | ACEITO RESIDUAL. Substituído pelos verdes por SHA de `165f4ff` (DA-2.118) |
| N-15 | 4 | Logs R6a e R6b da rodada 6 da NL-2.14 citados em `NL-2.14-rev-devops.md:272-273` não existem mais em disco (limpeza autorizada em 01/11/2026 00:13, registro linha 363) | ACEITO RESIDUAL (DA-2.124). Vale a citação do relatório |
| N-18 | 4 | NL-2.27 (`c97fa67`): a APROVA da rodada 4 (`NL-2.27-rev-seg.md:221`) cobre o delta de teste; o código foi dado como certo nas rodadas 2 e 3 | ACEITO (DA-2.123, CISO copiado no quadro) |
| N-16 | 4 | O quadro FECH-2-3 não cobre DA-2.113 a 2.124 (decisões de 03/11 posteriores à montagem às 10:17); a linha 388 repete a 387 | ABERTO no quadro. As decisões foram ratificadas na Q3. Remediação: atualizar o quadro ou anexar nota |
| N-23 | 4 | DA-2.119 a 2.123 não têm entrada própria no registro: a linha 595 resume o intervalo "DA-2.118 a DA-2.124" | ABERTO. Remediação: uma entrada por DA no registro |
| N-22 | 4 (revisão R2) | Achados A1 a A4 do revisor R2 (BAIXA): TOCTOU em `raizes_producao.cpp:42-43`; lambdas com referência em `transporte_https.cpp:237-268`; erro de rede mapeado para Tls em `transporte_https.cpp:319-323`; `validarPedido` sem recusa de `..` em `transporte_https.cpp:150-155` e `:175` | ABERTO. Vai à INBOX pela FECH-2-6, com nota para triagem do CISO na abertura da Onda 3 |
| N-07 | 3, 4 | Logs `NL-2.38-verde*.log` (portões) com SHA só no nome | ABERTO (herdado da parte A) |
| N-08 | 3, 4 | Registro (linha 183) chama de "exceção" a ordem que o líder classificou como "É ordem, não exceção" | ABERTO (herdado) |
| N-04 | 2, 3 | Caminho absoluto de máquina em `docs/pesquisa/onda1_leiaute.md:141` | ABERTO (herdado; não reconferido nesta passada) |
| N-05 | 3, 4 | R3-M19 sobrevivente em `NL-2.10c-rev-qa.md:44`, `:82` e `:135` | ABERTO (herdado) |
| N-10 | 4 | TODO sem linhas para NL-2.10d, NL-2.10f, NL-2.10i e NL-2.27b | FECHADO pela FECH-2-1 (linhas novas no `TODO.md` de `df229cc`) |
| A2-02 | 1 | Teste `xmlAlteradoDepoisDeAssinarReprova` sem controle positivo dentro da função (`tst_assinador_xml.cpp:427-434`) | ABERTO (herdado) |
| A2-03 | 1 | `ac584` citado no registro sem correspondência no blob | ABERTO (herdado) |
| A2-09 | 2 | `NL-2.2-imagens-verde-parcial.log` com o nome original | ABERTO (herdado) |
| A2-10 | 2 | Logs com `origem=arvore-de-trabalho` (9 arquivos) | ABERTO (herdado) |
| A2-12 | 2 | `src/foundation/log/log.cpp:50-52`, C1 em UTF-8 | ABERTO (herdado) |
| A2-14 | 3 | `tst_carregador_pkcs12.cpp` sem asserção do provider default | ABERTO (herdado) |

### 3.4 Fechados ou resolvidos

| ID | Descrição | Estado |
|---|---|---|
| N-09 | Dado que parecia pessoal em `parametros_municipais.md` e `emitir_teste.sh` (CRÍTICO da parte A) | FECHADO como falso positivo (registro, linha 580) |
| N-02 | Ordem de sequência, classificada pelo líder como "É ordem, não exceção" | RESOLVIDO |
| N-06 | Ratificação de decisões desde 01/11 03:29 | FECHADO pela Q2 (03/11 10:53) |
| A2-05 | DA-2.32 e DA-2.33 fora do registro | FECHADO (registro retroativo na linha 575; ratificação na Q2) |
| A2-06 | Verde NL-2.0 sem SHA | FECHADO por SHA `1384887` |
| A2-08 | Verde NL-2.0 | FECHADO |
| A2-11 | Mensagem de erro do transporte | FECHADO |
| P-04 | Risco residual de `settings.json` | ACEITO pelo líder ("Manter como está (Recomendado)") |
| N-19, N-20 | IMPORTANTES da revisão retroativa do N-01 | FECHADOS pela NL-2.15 (seção 3.2) |

### 3.5 Contagem final

- CRÍTICO aberto: **0**.
- IMPORTANTE aberto, bloqueante: **0**.
- IMPORTANTE aceito residual com controle: **3** (N-01, N-11, N-14).
- COSMÉTICO aberto: **14** (N-04, N-05, N-07, N-08, N-16, N-22, N-23, N-24 como desvio de processo, A2-02, A2-03, A2-09, A2-10, A2-12, A2-14).
- COSMÉTICO aceito residual: **4** (N-12, N-15, N-18, N-21).
- Fechados: N-09, N-10, N-13, N-02, N-06, A2-05, A2-06, A2-08, A2-11, P-04 (aceito), N-19, N-20.

---

## 4. Status proposto no TODO.md

Critério: DA-2.115 (Status ✅ quando há revisão que cobre o SHA mesclado, execução local adequada e CI verde no SHA final; Estado Auditado ✓ sem achado aberto ligado, ⚠ com COSMÉTICO, N-01 ou N-11 ligado). Aplicado às DA-2.122 a 2.125 da triagem, com as verificações desta passada.

| ID | Status proposto | Estado Auditado | Motivo |
|---|---|---|---|
| NL-2.0 | ✅ | ⚠ | DA-2.117 (registro, linha 17); `7bcf8c5` ancestral do alvo; verde por SHA `1384887`; CI do alvo |
| NL-2.1 | ✅ | ⚠ | DA-2.122: aceite do CTO (DA-2.26, `decisoes_autonomas_onda2.md:57-58`) sobre REPROVA da rodada 3, com limite declarado. Sem APROVA de revisor; ressalva registrada |
| NL-2.2 | ✅ | ⚠ | Herdado da parte A (A2-09, A2-10) |
| NL-2.3 | ✅ | ⚠ | APROVA `NL-2.3-rev2-revisao.log:21` no SHA `265dd3f` = `^2`; CI cancelled no merge (N-01) |
| NL-2.4 | ✅ | ✓ | Herdado da parte A |
| NL-2.5 | ✅ | ⚠ | FECH-2-R2 APROVA; N-14 aceito residual (DA-2.121) |
| NL-2.6 | ✅ | ⚠ | Merge `b34e3b3` com CI vermelho, corrigido por `102c905` (N-01) |
| NL-2.7 | ✅ | ✓ | Herdado da parte A |
| NL-2.8 | ✅ | ⚠ | Merge `062de86` com CI vermelho, corrigido por `97b429d` (N-01) |
| NL-2.9 | ✅ | ⚠ | DA-2.122. Conferido: `NL-2.9-r2-verde-fedora44-605bc48.log`, linha 1 `SHA: 605bc488...`, `origem=archive`, `100% tests passed, 0 tests failed out of 342`, `NL-RC=0` |
| NL-2.9b | ✅ | ✓ | Herdado da parte A |
| NL-2.10a | ✅ | ⚠ | APROVA da rodada 2 em `NL-2.10a-rev-qa.md:87-88`, SHA `88debb7`, ancestral de `85a26e4` (`^2` de `d4ad607`) e de `12934a2` (`^2` de `7ac5716`). Deltas são só de teste, servidor falso e documentação, sem `src/`. Ressalva: o SHA aprovado não é o `^2` |
| NL-2.10b | ✅ | ✓ | Herdado da parte A |
| NL-2.10c | ✅ | ⚠ | N-05 (COSMÉTICO) |
| NL-2.10d | ✅ | ⚠ | DA-2.124: conteúdo não chega ao SHA final (superado pela NL-2.10j). N-11 |
| NL-2.10e | ✅ | ⚠ | N-01 (`3298206` com CI vermelho, corrigido em `22cdb30`) |
| NL-2.10f | ✅ | ⚠ | DA-2.124: superado. N-11 |
| NL-2.10g | ✅ | ✓ | Herdado: revisão nas rodadas 6 e 8 antes do merge; ensaio de script; CI do SHA final |
| NL-2.10h | ✅ | ⚠ | H2 revisado por FECH-2-R1 (APROVA rodada 2, `55f9cfc`); H1 e H4 como antes. N-11 aceito residual |
| NL-2.10i | ✅ | ⚠ | DA-2.124: superado pela NL-2.10j. N-11 |
| NL-2.10j | ✅ | ✓ | J2 e J3 revisadas antes dos merges; ensaio "132/0"; CI do SHA final |
| NL-2.10 | ✅ | ⚠ | Condições (8) e (9) ATENDIDAS. N-13 fechado em `df229cc`; ressalva N-24 |
| NL-2.11 | ✅ | ⚠ | N-01 ligado (CI vermelho nos merges). A revisão retroativa do `018957b` (N-19, N-20) está fechada pela NL-2.15 (`NL-2.15-rev-qa.md`, `NL-2.15-rev-devops.md`, `NL-2.15-ci-rev-qa.md`) |
| NL-2.12 | ✅ | ⚠ | DA-2.122. Alvo `bc7d435` = `^2`, m3 MORTO. Sem linha de veredito (ressalva) |
| NL-2.13 | ✅ | ⚠ | FECH-2-R2 APROVA (m2 MORTO em `5bf6650`); N-14 aceito residual |
| NL-2.14 | ✅ | ⚠ | APROVA da rodada 6 em `ab32d13` = `^2`; CI success no merge; N-15 (logs ausentes) |
| NL-2.15 | ✅ | ⚠ | `c804b95^2` = `0828bd1` revisado por QA (APROVA COM RESSALVAS, `NL-2.15-rev-qa.md:5`); `9b3a97e^2` = `1d2e877` revisado por CI (APROVA, `NL-2.15-ci-rev-qa.md:4`); devops APROVA (`NL-2.15-rev-devops.md:3`). N-01 ligado |
| NL-2.21 | ✅ | ✓ | Herdado da parte A |
| NL-2.27 | ✅ | ⚠ | DA-2.123: encadeamento de revisão (N-18); N-01 (`c97fa67` vermelho); N-11 (hora da APROVA não registrada) |
| NL-2.27b | ✅ | ⚠ | `5d44ec9` aprovado antes do merge `6c6118a`; N-01 (`dbd6e40` vermelho) |
| NL-2.38 | ✅ | ⚠ | DA-2.124: autoteste do portão com estreia vermelha no QA (`NL-2.38-qa.md`, antes do merge), e o job "camadas" passa no SHA final (N-07) |
| NL-2.F | ✅ | ⚠ | 12 de 12 condições ATENDIDAS; nenhuma linha da W2 em 🔍. Ressalvas: N-16, N-23, N-24 |

Contagem da W2: ✅ ✓ 7 (NL-2.4, 2.7, 2.9b, 2.10b, 2.10g, 2.10j, 2.21); ✅ ⚠ 25; 🔍 0. Linhas NL-2.16 a NL-2.20 ficam fora (DA-2.116: são INBOX).

---

## 5. Decisões do líder e do CTO citadas

Verbatim, com data, como estão no registro e nos documentos do fechamento.

- **01/11/2026 07:29** (registro, linha 382; `ratificacao-onda2.md:267`): "É ordem, não exceção (Recomendado)" (N-02); "Manter como está (Recomendado)" (P-04); "Ratifico todas (Recomendado)" (N-06, cobre decisões até 01/11 03:29); "Adotar na Onda 3 (Recomendado)" (N-01).
- **03/11/2026 09:33** (`fechamento-onda2-plano.md:176`): "autorizo tag/merge/push", condicionada às condições (1) a (12).
- **03/11/2026** (Q1, emenda ao item 9 da definição, `docs/superpowers/plans/2026-10-25-notalivre-plano.md`): "Não exercida (Recomendado)". Uma INFERÊNCIA que a emissão de teste não exercitou fica marcada NÃO EXERCIDA, com a fatia de destino.
- **03/11/2026 10:53** (Q2, registro, linha 579): "Ratificar todas (Recomendado)", que cobre as 84 decisões do quadro FECH-2-3 (DA-2.68 a 2.112, mais DA-2.32 e DA-2.33).
- **03/11/2026 13:30** (Q3, registro, linha 596): "Ratificar todas (Recomendado)", que ratifica DA-2.113 a DA-2.124. Mesma linha: Q4, "Manter (Recomendado)", sobre regime e município no documento público.
- **03/11/2026 13:56** (triagem p4b, DA-2.125, a ratificar): N-21 não bloqueia, rebaixado a COSMÉTICO com destino na NL-3.7.
- **Desvio sem decisão do líder no registro:** leitura da devolução pelo CTO (linha 595; N-24). Pendente de ciência do líder.

---

## 6. Controles e limites desta passada

- O auditor não leu a pasta de devolução, o certificado do líder nem `ultimo-ndps.txt`. A medição de DV do N-24 não foi reproduzida por este auditor.
- Revisões retroativas FECH-2-R1 e FECH-2-R2 e revisões da NL-2.15 são de outros agentes. Conferi por amostra: o blob `55f9cfc` (R1); os cabeçalhos e os SHAs dos logs de mutante de R2 (`NL-RC=8`, `origem=archive`); os SHAs `^2` de `c804b95` e `9b3a97e` contra os SHAs revisados pela NL-2.15; a lista e o portão do Flatpak e o teste `corpoEscritoEConexaoFechadaConsultaAntesDeResponder` no blob `df229cc`.
- O log do run B2 foi lido por pipe e gravado só em um scratch do próprio auditor, fora de `/var/tmp/notalivre-logs/`.
- Rascunho do auditor (`/var/tmp/notalivre-fech25`) foi removido no passo 10 (primeira rodada, 13:53).
- Parte A e parte B preservadas em `/var/tmp/notalivre-logs/auditoria-onda2-passada4.md`.
