# Auditoria independente de fechamento da Onda 0 (NotaLivre)

**Auditor:** internal-auditor, rodada independente (sem implementação e sem escrita em git).
**Data da medição:** 25/10/2026.
**Branch:** `onda-0`. HEAD local e `origin/onda-0` no mesmo SHA `60748ca8a4a55b29c4473e90767699abac1c0d8c` (`git ls-remote`).
**Plano:** `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, seção "Onda 0" e a "Definição de fechamento da Onda 0" (itens 1 a 8).
**Fontes de CI:** runs `40236959902` (9 jobs, SHA `60748ca`) e `40235050696` (6 jobs, SHA `1485c86`), lidos com `gh run view --log` (log de dentro do processo, L-49).

---

## 1. Veredito

**A Onda 0 não pode fechar hoje.** Dois itens da definição não são atendidos pela letra, e ambos são falta de *registro*, não de *comportamento*: o vermelho de sabotagem existe nos dois lados (contêiner e CI), mas falta o SHA no log local (A0-01) e falta a comparação escrita das listas de jobs (A0-02). Por isso o porte da remediação é pequeno: registrar e rerodar, sem mudar portão.

- Nenhum achado CRÍTICO. O item 6 (sem CRÍTICO) é atendido por este relatório.
- Bloqueiam o fechamento: **item 2** (falta o SHA na sabotagem local) e **item 4** (espelho local sem lista nem comparação registrada).
- Recomendados antes do fechamento, por serem aceites do próprio plano ou da L-36: **A0-04** (documento de medição desatualizado), **A0-05** (portão do build nativo nunca visto vermelho no CI) e **A0-15** (o ramo de verde falso do harness nunca foi visto reprovando).
- Contagem por severidade: CRÍTICO 0; IMPORTANTE 8 (A0-01 a A0-07 e A0-15); COSMÉTICO 7 (A0-08 a A0-14).

## 2. Veredito item a item

| # | Item da definição | Veredito | Evidência |
|---|---|---|---|
| 1 | Quatro jobs nativos compilam e rodam `tst_sanidade` com Qt 6.12 | **SIM** | Run 40236959902: `nativo (fedora44)`, `(ubuntu26.04)`, `(manjaro)` e `(mint22.3)` com conclusão `success`. Em cada log, `Test #1: tst_sanidade ... Passed` e 100% passed. O CMake achou o Qt no prefixo `qt/6.12.0/gcc_64` do runner, nos quatro. O run 40235050696 também tem os quatro verdes. |
| 2 | A2, gitleaks e travessão vistos vermelhos por sabotagem, no contêiner local e no job `mutacao`, com SHA | **NÃO** | CI: SIM, com SHA. O log do job `mutacao` (run 40236959902) faz checkout de `60748ca` e imprime `portao=a2 rc=1 motivo=ok`, `gitleaks rc=1 motivo=ok`, `travessao rc=1 motivo=ok`, `flatpak611 rc=1 motivo=ok`. Local: há vermelho em contêiner (A0-01), mas nenhum log local cita SHA. |
| 3 | Resultado de D0.5 registrado em `docs/pesquisa/ci_medicao.md` | **SIM** (pela letra) | Seção "Flatpak (NL-0.5)" com "Ponto de decisão D0.5", plano B padrão. Ressalva: o texto diz que o CI remoto não rodou e não traz o link do job, exigido pelo aceite NL-0.5 (plano, linha 210). Ver A0-04. |
| 4 | Lista de jobs do espelho local bate com o workflow e com a última execução (L-36) | **NÃO** | Workflow (9 jobs) e run 40236959902 (9 jobs) batem pelos nomes. Não existe lista do espelho local nem registro da comparação. Flatpak e smoke-host só têm comandos manuais (`docs/pesquisa/ci_medicao.md`, linhas 218 a 226). Ver A0-02. |
| 5 | Toda fatia commitada com ID e marcador de verificação, empurrada para `onda-0` | **SIM** | Seis commits NL-0 (`6f7859e`, `674255d`, `f797cda`, `14b904d`, `1485c86`, `60748ca`) com `Refs: NL-0.x` e a coluna Status trocada para "Pendente verificação" no mesmo commit. `ea9e1a9` é a movimentação para a INBOX. `git merge-base --is-ancestor` confirma que os oito commits estão em `origin/onda-0`. |
| 6 | Relatório do internal-auditor sem CRÍTICO | **SIM** | Nenhum achado CRÍTICO (seção 4). |
| 7 | CI verde na `onda-0` | **SIM** | Run 40236959902 (`60748ca`) com 9 jobs `success`. Run 40235050696 (`1485c86`) com 6 jobs `success`. Verde é uma linha entre outras, conforme a L-24. |
| 8 | Merge em `main` só com autorização do líder | **SIM** (não houve merge) | `main` = `06d4022`. `git rev-list --count main..onda-0` = 8, e `onda-0..main` = 0. Sem PR no repositório e sem tags. |

## 3. Método

- Leitura dos logs de CI com `gh run view <id> --log` (conta `petrinhu`), não do painel. Parse por job e por step.
- Leitura do workflow `.github/workflows/ci.yml` e do script `tools/mutacao/sabotar_portoes.sh`.
- Leitura de `docs/pesquisa/ci_medicao.md`, do `TODO.md`, do `CLAUDE.md` e dos logs locais `NL-0.x` e `REV-NL-0.x` (pasta de logs fora do repositório).
- Busca do ramo de verde falso do harness (`NÃO reprovaram`, `verde falso`) em todos os logs de CI e em todos os logs locais.
- Histórico: `git log --all -p` (8841 linhas de diff, 12 commits) e gitleaks em modo git e dir, com o `.gitleaks.toml` do projeto. Relatórios gravados fora do repositório.
- Conferência de SPDX em todo arquivo rastreado de código, e de travessão em todo arquivo textual rastreado.
- `LICENSE` comparado byte a byte com o texto oficial publicado pela FSF (`curl`, `sha256sum` e `cmp`).
- Nenhuma sabotagem foi reexecutada por mim. As contagens e vereditos vêm dos logs.

## 4. Achados

### 4.1 CRÍTICO

Nenhum.

### 4.2 IMPORTANTE

| ID | Achado | Evidência | Remediação | Re-teste | Estado |
|---|---|---|---|---|---|
| A0-01 | Sabotagem local de A2, gitleaks e travessão sem SHA de commit. Só o CI registra o SHA (`60748ca`). O log local de `flatpak` roda sobre a árvore de trabalho, fora de contêiner. | `NL-0.3-rev2-green-v3.log`, controle b (VIOLACAO `src/back/camada.cpp:8`); `NL-0.6-rev3-verde.log`, linhas 60, 66, 40, 41, 49, 53 (S2, S3, T2, T3, T11, T14); `NL-0.5-mutacao-local.log` (sem SHA). Plano, linha 193 e linha 220 (item 2). | Repetir a sabotagem no contêiner sobre `git archive 60748ca`, gravar o SHA no log e em `docs/pesquisa/ci_medicao.md`. | Log com SHA e rc=1 com motivo esperado nos três portões. | Aberto (bloqueia item 2) |
| A0-02 | Espelho local sem lista versionada e sem registro de comparação com o workflow e com o run. Flatpak e smoke-host não têm script local. | `tools/conteiner/build_local.sh` cobre o nativo; `tools/portoes/rodar_higiene_conteiner.sh`, `autoteste_camadas.sh` e `tools/mutacao/sabotar_portoes.sh` cobrem os demais. Flatpak e smoke-host só em `docs/pesquisa/ci_medicao.md`, linhas 218 a 226. Plano, linha 525. | Tabela job para comando local para job do CI, com a comparação registrada contra o run id. Ou um script de espelho que imprima a lista. | Lista gerada bate com os nove nomes do run. | Aberto (bloqueia item 4) |
| A0-03 | Desvio não registrado no gatilho do job `mutacao`. O plano diz "disparado por `workflow_dispatch` e pelos arquivos `tests/mutacoes/`". O workflow roda a cada push e usa `tools/mutacao/`. | `.github/workflows/ci.yml`, linhas 278 e 294; plano, linha 519. | Registrar o desvio no `TODO.md` (L-73), ou ajustar o plano. Rodar a cada push é mais estrito. Se o líder quiser mudar o gatilho, a decisão é dele. | Não aplicável após registro. | Aberto |
| A0-04 | Documentação com afirmações falsas sobre o CI. Diz que o CI remoto não rodou. Não traz o link do log do job (aceite NL-0.5). Diz que a matriz das quatro distros saiu do `ci.yml`, mas o `ci.yml` tem a matriz. | `docs/pesquisa/ci_medicao.md`, linhas 5, 113, 127, 176, 177, 190, 202; `.github/workflows/ci.yml`, linha 8 (cabeçalho "NÃO rodado"); plano, linha 210. | Atualizar com os runs `40235050696` e `40236959902` e com o link de cada job. Corrigir o cabeçalho do `ci.yml` e a linha 177 do documento. | Leitura do documento contra `gh run view`. | Aberto (recomendado antes do fechamento) |
| A0-05 | O portão de build nativo (`ctest` de `tst_sanidade`) nunca foi visto vermelho no CI. Só a sabotagem local existe. O plano exige "nos dois lados". O job `mutacao` não cobre o nativo. | `.github/workflows/ci.yml`, linhas 87 a 89 e 290 a 294; logs locais `NL-0.4-sabotagem-*.log`; plano, linha 201. | Acrescentar ao job `mutacao` a sabotagem de estreia do `tst_sanidade` em cópia, com verificação do motivo, e ver o vermelho no CI. | Run com a sabotagem vista vermelha, citando o SHA. | Aberto (recomendado antes do fechamento; L-36) |
| A0-06 | Cabeçalho SPDX faltando. O plano exige que a conferência seja feita por portão, e não existe portão de SPDX. Três fixtures QML não têm cabeçalho. `CMakePresets.json` também não tem (JSON não tem comentário). | Fixtures: `tests/fixtures/higiene/limpo.qml`, `com_em_dash.qml` e `com_en_dash.qml` (a linha 1 é `import QtQuick`). Nenhum portão SPDX em `tools/portoes/` nem em `.github/`. Plano, linha 184 (NL-0.2). | Criar portão SPDX com sabotagem de estreia (L-36) e varredura não vazia. Cabeçalho nas fixtures ou exceção estreita. Para o JSON, arquivo `.license` ou `REUSE.toml`. | Sabotagem vista vermelha; contagem de arquivos analisados igual à de encontrados. | Aberto |
| A0-07 | Job `flatpak` usa imagem de terceiros por tag (`kde-6.11`), não por digest, com `--privileged`. A action monta o socket do Docker do runner. As distros estão fixadas por digest, então há inconsistência. O risco não está registrado como desvio com controles compensatórios. | `.github/workflows/ci.yml`, linhas 189 a 191. Log do job `flatpak` (run 40236959902): criação do contêiner com `--privileged` e com o socket do Docker montado. L-77. | Fixar por digest. Registrar ADR de desvio com controles e forma de verificação periódica. Decidir se `pull_request` de fork deve rodar o job privilegiado. | Digest conferido no log do job. | Aberto (antes da Onda 1) |
| A0-15 | O ramo de verde falso do próprio harness nunca foi exercitado. O `sabotar_portoes.sh` tem um ramo que reprova o veredito se um portão não reprovar pelo motivo esperado. Nenhum log de CI nem local mostra esse ramo. Um detector de verde falso que nunca detectou nada é o que a L-36 veta. | `tools/mutacao/sabotar_portoes.sh`, linhas 49 a 52 e 108 a 113. Busca por `NÃO reprovaram` e `verde falso` em `run-40236959902` e `run-40235050696` (zero) e em todos os logs locais (zero). L-36. | Sabotar o harness numa cópia: desligar um portão (por exemplo, trocar o motivo esperado) e ver o job `mutacao` terminar com `exit 1` e a linha de veredito de falha. | Log do job vermelho por ausência de motivo, com `exit 1`, citando o SHA. | Aberto (recomendado antes do fechamento; L-36) |

### 4.3 COSMÉTICO

| ID | Achado | Evidência | Remediação | Estado |
|---|---|---|---|---|
| A0-08 | Caminho absoluto fixo do host em configuração versionada: `binaryDir` do preset e constantes de caminho nos scripts de contêiner, apontando para a pasta temporária do sistema. A convenção foi fixada pelo líder (ponto L7) e o CI roda com ela nos quatro contêineres. É decisão de padrão, não defeito de código. | `CMakePresets.json`, linha 9; `tools/conteiner/build_local.sh`, linhas 37 a 39; `tools/portoes/rodar_higiene_conteiner.sh`, linhas 21 e 25; `tools/conteiner/medir_versoes.sh`, linhas 68 a 78. | Decisão do líder: manter o padrão ou tornar `binaryDir` relativo (`${sourceDir}/build`, já ignorado pelo `.gitignore`). | Aberto (decisão do líder) |
| A0-09 | A sabotagem copia a árvore de trabalho e não o commit. No CI o checkout é limpo, então é equivalente. Localmente pode incluir alteração não commitada (L-27 pede o blob commitado). | `tools/mutacao/sabotar_portoes.sh`, linhas 30 a 35. | Localmente, usar `git archive <SHA>` como raiz. | Aberto |
| A0-10 | Saída ambígua do portão de segredos: `encontrados=73` com rc=0 num controle positivo de árvore limpa. | `NL-0.6-rev3-verde.log`, linha 58. | Separar `analisados` e `encontrados` na saída (L-36). | Aberto |
| A0-11 | `.claude/` não rastreado e fora do `.gitignore`. Contém `scheduled_tasks.lock`. Risco de commit acidental de estado de ferramenta. | `git status`: `?? .claude/`; `.gitignore`. | Incluir `.claude/` no `.gitignore`. | Aberto |
| A0-12 | Arquivos versionados fora do escopo de texto do usuário, cuja permanência no repositório público precisa de confirmação: `prompt-emissor-nfse-nacional.md` (raiz, com 2 travessões U+2014) e `.bigtech-porte` (marcador de governança interna). | `git ls-files`; `tools/portoes/higiene_travessao.py`, linhas 8 e 9. | Confirmar com o líder se ficam no repositório público. | Aberto |
| A0-13 | Dado de autoria e de titular a confirmar. Os commits usam `petrus@drpetrus.top` (visível no histórico público). O titular `Petrus Alves da Silva Costa` aparece em 46 cabeçalhos e no `LICENSE`. O plano pede confirmar o nome exato com o líder (NL-0.1), e não há registro dessa confirmação. | `git log --all`; cabeçalhos SPDX. | Confirmar com o líder e registrar no `CLAUDE.md`. Sem mudar identidade git (L-30). | Aberto |
| A0-14 | Nome de pasta pessoal de sincronização num comentário versionado. Não é dado sensível. | `tools/conteiner/build_local.sh`, linha 34. | Trocar por descrição genérica. | Aberto |

## 5. Varreduras

| Varredura | Resultado |
|---|---|
| gitleaks git (histórico, 12 commits, cerca de 720 KB) | Nenhum vazamento. |
| gitleaks dir (árvore rastreada e não rastreada) | Nenhum achado. |
| Termos no histórico (`git log --all -p`, com `-i`): senha, password, token, secret | Ocorrências em texto de documentação, nas regras do próprio gitleaks e em fixtures sintéticas. Nenhum valor real. |
| `BEGIN PRIVATE KEY`, `BEGIN RSA`, prefixos de token de GitHub e AWS | Zero no histórico e na árvore. |
| Caminho de usuário (prefixo de pasta home) | Só em URLs de fonte em `docs/pesquisa/ibscbs.md` (linhas 95 e 101). Nenhum caminho de usuário. |
| Caminho da pasta de builds do agente de execução | Zero no histórico e na árvore. |
| Caminho absoluto fixo do host em arquivo versionado | Presente, na pasta temporária do sistema. Ver A0-08. |
| Nome da pasta pessoal de sincronização | Duas ocorrências no histórico e uma na árvore, num comentário. Ver A0-14. |
| Travessão U+2014 e meia-risca U+2013 em texto de usuário (`README.md`, `LICENSE`, `CLAUDE.md`, `docs/` público) | Zero. |
| Travessão em arquivos fora de escopo de usuário | `TODO.md` (131, interno), `prompt-emissor-nfse-nacional.md` (2, ver A0-12), fixtures de higiene (1 e 1, intencionais e permitidas por allowlist). |
| SPDX em código rastreado (50 arquivos avaliados) | 46 com `GPL-3.0-or-later` e titular. 4 sem (ver A0-06). |
| `LICENSE` contra o texto oficial da FSF | Idêntico byte a byte (sha256 `3972dc97...`, medido em 25/10/2026). |

## 6. Conformidade observada (sem achado)

- Actions fixadas por SHA de commit (`.github/workflows/ci.yml`, linhas 47, 61, 110, 158, 193, 199, 226 e 285).
- `permissions: contents: read`, `concurrency` com cancelamento, e `fetch-depth: 0` no job `higiene`, necessário para o modo git do gitleaks.
- Piso de varredura não vazio nos logs de CI: camadas com `encontrados=10 analisados=10`, higiene com `encontrados_ui=12 encontrados_docs=1`.
- Vermelho de sabotagem visto no CI, com o motivo esperado, no job `mutacao`.
- L-12: existem logs `REV-NL-0.x` de execução de sabotagem por revisor, incluindo a reprovação do símbolo `qSharedPointerStaticCast` sob o SDK 6.11. A identidade dos agentes não é verificável só pelos logs.
- `LICENSE` idêntico ao texto oficial da FSF.

## 7. Limites desta auditoria

- Não reexecutei sabotagens nem builds. As contagens vêm dos logs.
- Não tenho acesso ao `GITHUB_STEP_SUMMARY` dos jobs. A evidência de versões é o log bruto.
- O nome exato do titular depende de confirmação do líder (A0-13).

## 8. Plano de remediação e re-teste

| Ordem | Ação | Achados | Resultado esperado |
|---|---|---|---|
| R1 | Repetir a sabotagem de A2, gitleaks e travessão no contêiner sobre `git archive 60748ca`, com SHA no log. | A0-01 | Item 2 passa a SIM. |
| R2 | Registrar a lista do espelho local, comparar com o run `40236959902` e registrar o resultado. | A0-02, A0-03 | Item 4 passa a SIM. |
| R3 | Atualizar `docs/pesquisa/ci_medicao.md` e o cabeçalho do `ci.yml` com os runs e links. | A0-04 | Documento aderente ao CI. |
| R4 | Sabotagem de estreia do `tst_sanidade` no job `mutacao`, vista vermelha no CI. Sabotagem do próprio harness, vista reprovar com `exit 1`. | A0-05, A0-15 | Aceite NL-0.4 nos dois lados, e L-36 fechada no harness. |
| R5 | Portão de SPDX com sabotagem de estreia, cabeçalhos nas fixtures e tratamento do JSON. | A0-06 | Aceite NL-0.2 por portão. |
| R6 | Imagem `flatpak` por digest e ADR de desvio (L-77). | A0-07 | Antes da Onda 1. |
| R7 | Itens COSMÉTICOS: entrada na INBOX do `TODO.md` (L-63), sem executar agora. | A0-09 a A0-14 | Registro. |
| R8 | Decisões do líder: padrão do caminho local, permanência de arquivos no repositório público, nome do titular e gatilho do `mutacao`. | A0-08, A0-12, A0-13, A0-03 | Decisão registrada. |

**Estado Auditado sugerido ao main** (o main grava no `TODO.md`; esta auditoria não edita o arquivo): NL-0.2, NL-0.3, NL-0.4, NL-0.5 e NL-0.6 em estado de aviso, pelos achados acima. NL-0.1 fica como não auditado nesta rodada (o conteúdo do canon não foi lido em profundidade; a licença foi conferida por hash).

**Entrega do líder:** a decisão de fechar, o gatilho do `mutacao` (A0-03), o caminho padrão local (A0-08), o nome do titular (A0-13) e a permanência de arquivos no repositório público (A0-12) são decisões dele, levadas por AskUserQuestion.

---

## Re-teste 25/10/2026

**Escopo:** HEAD `fb264f5` (remediação) em `onda-0`, confirmado por `git ls-remote origin` (`refs/heads/onda-0` = `fb264f5`). `main` segue em `06d4022`. Não há tag local nem remota. Leitura apenas, sem git de escrita.
**CI:** run `40241779760`, SHA `fb264f5`, 9 jobs, todos `success`. Logs lidos por `gh run view --log` (L-49).
**Logs locais:** pasta de logs da máquina, fora do repositório (R1, R2, R-autoteste e espelho-01 a espelho-09).

### Itens da definição

| # | Veredito | Evidência |
|---|---|---|
| 1 | **SIM** | Run 40241779760: os quatro nativos `success`. Em cada um, `Test #1 tst_sanidade Passed` (100%) e `sabotagem=tst_sanidade build_rc=0 ctest_rc=8 motivo=ok`, ou seja, sabotagem vermelha pelo motivo `nomeCamadaDaBaseConfere`. Qt 6.12.0 no prefixo do runner. |
| 2 | **NÃO** (falta uma execução) | CI com SHA `fb264f5`: job `mutacao` com `fonte=git-archive sha=fb264f5`, e `a2`, `gitleaks`, `travessao`, `spdx` e `flatpak611` com `rc=1 motivo=ok`. Local: o R1 (A2, gitleaks e travessão, `rc=1 motivo=ok`) foi rodado no SHA `0cd5a29`, commit anterior. O espelho local 9/9 verde foi rodado em árvore suja (`0cd5a29+sujo`). Nenhuma execução local está no SHA `fb264f5`. Entre os dois SHAs mudaram `higiene_segredos.sh`, `higiene_travessao.py` e `sabotar_portoes.sh`, então o registro local não cobre o SHA fechado. |
| 3 | **SIM** (pela letra) | D0.5 registrado em `docs/pesquisa/ci_medicao.md`. O texto ainda está defasado (A0-04). |
| 4 | **SIM** | Os nove nomes de `tools/espelho_local.txt` são idênticos aos nove nomes do run 40241779760 (conferido por `diff`). O comparador tem controle: `R2-controle-sabotagem-comparador.log` reprova quando falta um job. O registro automático (`R2-comparar-jobs.log`) cita o run 40236959902; convém repetir contra 40241779760, mas não bloqueia. |
| 5 | **SIM** | Os seis commits NL-0 têm ID e o `TODO.md` marca NL-0.1 a NL-0.6 como "Pendente verificação". `fb264f5` está no remoto. Ressalva: nenhum achado A0 foi para a INBOX (A0-17). |
| 6 | **SIM** | Nenhum CRÍTICO nesta re-auditoria. |
| 7 | **SIM** | Run 40241779760 (`fb264f5`), 9 jobs `success`. |
| 8 | **SIM** | Sem merge (`main` = `06d4022`). Sem tag. `v0.1.0.0` está no `CMakeLists.txt` e no commit; a tag depende do aval do líder. |

**A Onda 0 pode fechar hoje?** **Não.** Faltam a execução local do item 2 no SHA `fb264f5` (A0-01) e a correção do documento de medição (A0-04). O item 2 depende de uma execução local: repetir o R1 com `git archive fb264f5` no contêiner. O A0-16 precisa da ratificação do líder.

**Rodada do espelho:** a primeira rodada teve três jobs com `rc=1` (higiene, camadas e smoke), por uma trava com aninhamento que travava. A rodada final (`R2-espelho-final.log`) ficou verde após a correção da trava (`R-autoteste-trava.log`, T1 e T2 verdes).

### Achados

| ID | Severidade | Estado | Reavaliação |
|---|---|---|---|
| A0-01 | IMPORTANTE | **Aberto** | Local no SHA `0cd5a29`, com `git archive`. Falta rodar no SHA `fb264f5` e registrar o SHA. |
| A0-02 | IMPORTANTE | **Fechado** | `tools/espelho_local.txt` com 9 linhas; comparador com controle; nomes idênticos ao run atual. Espelho 9/9 verde em árvore suja (ver item 2). |
| A0-03 | IMPORTANTE | **Fechado** | Decisão do líder (`mutacao` a cada push) registrada no plano; o diff de `fb264f5` não adiciona filtro de caminho ao job. |
| A0-04 | IMPORTANTE | **Aberto** | `docs/pesquisa/ci_medicao.md` linhas 5, 113 e 190 e `ci.yml` linha 8 ainda dizem que o CI não rodou. A seção nova de `fb264f5` cita só o run 40236959902; faltam os runs 40241779760 e 40235050696 com links de job. |
| A0-05 | IMPORTANTE | **Fechado** | Step `sabotagem_sanidade` nos quatro nativos, vermelho pelo motivo, `ctest_rc=8` (run 40241779760). |
| A0-06 | IMPORTANTE | **Fechado** | `spdx.py`: 61 arquivos de código, 0 violações, 2 exceções declaradas (JSON). Autoteste SPDX com 6 controles verdes. Fixtures com cabeçalho. Sabotagem `spdx rc=1 motivo=ok` no `mutacao`. |
| A0-07 | IMPORTANTE | **Aceito com decisão** | Imagem por digest `b0a8a2…` confirmada no log do job `flatpak`. ADR 0001 aceito. `fixacao_ci.py`: `privilegiados=3 violacoes=0` em cada push. Segue aberta a pendência do líder sobre `pull_request` de fork (C6 do ADR). |
| A0-08 | COSMÉTICO | **Fechado** | Decisão do líder implementada: `binaryDir` por `$env{NL_TMP_BASE}` (`CMakePresets.json`, linha 9), com padrão de pasta temporária do sistema no CI. Efeito: o preset só existe com a variável definida (linha 10). |
| A0-09 | COSMÉTICO | **Fechado** | `sabotar_portoes.sh` extrai do `git archive` (saída `fonte=git-archive`). |
| A0-10 | COSMÉTICO | **Fechado** | Saída do gitleaks passou a `encontrados=96 analisados=96 segredos=0`. |
| A0-11 | COSMÉTICO | **Fechado** | `.gitignore` com `.claude/` (commit `f9f20bc`). `git status` limpo. |
| A0-12 | COSMÉTICO | **Aberto (parcial)** | Brief movido para `docs/origem/` (decisão do líder, implementada). `.bigtech-porte` segue rastreado, sem decisão registrada. |
| A0-13 | COSMÉTICO | **Aceito com decisão** | Nome e e-mail confirmados pelo líder. Falta o registro no `CLAUDE.md` (L-68): a busca não encontrou nada. |
| A0-14 | COSMÉTICO | **Fechado** | `tools/conteiner/build_local.sh` linha 34 sem nome de pasta pessoal. |
| A0-15 | IMPORTANTE | **Fechado** | Controle do harness no job `mutacao`: `rc=1`, "1 de 1 portão(ões) NÃO reprovaram (verde falso)", step passa com `rc=1`. Local: `espelho-09` com `controle=1` e "JOB mutacao: VERDE". |
| A0-16 | IMPORTANTE | **Aberto (decisão do líder)** | Novo. `tools/portoes/higiene_travessao.py` passou a excluir `docs/origem/`, `docs/auditoria/` e `docs/adr/` do grupo DOCS (`fb264f5`). Decisão do orquestrador, sem ratificação. A exclusão é por pasta inteira: documento novo com travessão nessas pastas passa. A justificativa escrita existe no código (L-36 exige escopo justificado); falta o aval do líder. Remediação: ratificar, ou restringir a arquivos nomeados (por exemplo, só o brief de origem). |
| A0-17 | COSMÉTICO | **Aberto (processo)** | Novo. Nenhum A0-xx está no `TODO.md` (INBOX, L-63). Remediação: registrar A0-01 a A0-16. |

### Remediação restante para fechar

1. Repetir o R1 com `git archive fb264f5` no contêiner e registrar o SHA (fecha A0-01 e item 2).
2. Corrigir `docs/pesquisa/ci_medicao.md` (linhas 5, 113 e 190) e `ci.yml` linha 8, citando os runs 40235050696, 40236959902 e 40241779760 com links de job (fecha A0-04).
3. Líder ratifica ou restringe a exclusão do gate de travessão (A0-16).
4. Registrar A0-01 a A0-16 na INBOX do `TODO.md` (A0-17).
5. Tag `v0.1.0.0` só com aval do líder. Não é critério dos itens 1 a 8.

**Limites desta re-auditoria:** não reexecutei sabotagens nem builds; os resultados vêm dos logs do CI e dos logs locais. Os logs locais do espelho e do R1 registram SHAs anteriores a `fb264f5`, e a conclusão do item 2 depende disso.
