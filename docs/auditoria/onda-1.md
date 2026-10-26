# Auditoria independente de fechamento da Onda 1 (NotaLivre)

**Auditor:** internal-auditor, rodada independente (sem implementação, sem escrita em git).
**Data da medição:** 26/10/2026 (início às 04:04, horário de Brasília).
**Branch:** `onda-1`. HEAD `7409319afc92bcb99503190e4b94299e171e749c`, igual ao `refs/heads/onda-1` do `origin` (`git ls-remote origin onda-1`).
**Plano:** `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, seção "Onda 1" e "Definição de fechamento da Onda 1" (linha 433, itens 1 a 11).
**Escopo deste capítulo:** commits `main..onda-1` (25 commits), lidos a partir dos blobs do HEAD. A árvore de trabalho está suja com trabalho não commitado da NL-1.7 (`apps/cli/comando_dps_validar.*`, `src/mid/composicao/ler_*.cpp` e afins); esse conteúdo não foi auditado e não entra no veredito.
**Evidências:** logs de vermelho e verde, revisões, `decisoes_autonomas_onda1.md` e `NL-1.8-relatorio-captura.md` em `/var/tmp/notalivre-logs/`, com os PNGs em `NL-1.8-captura/`.
**CI:** lido de dentro do runner com `gh run view --log` (L-49), não do painel.

---

## 1. Veredito

**A Onda 1 NÃO PODE FECHAR hoje.** Dos 11 itens da definição de fechamento: 7 ATENDE (o item 1 com ressalva, e os itens 3, 6, 7, 8, 9 e 11), 2 NÃO ATENDE (itens 4 e 5) e 2 PENDENTES da NL-1.7 (itens 2 e 10, sem veredito por ordem de serviço).

- **Item 1 (ATENDE com ressalva).** Todo vermelho amostrado vem antes do verde. Cada fatia tem verde limpo na CI do SHA que a contém (tabela no item 1). Os logs locais de NL-1.0c, NL-1.1, NL-1.2 e NL-1.8 foram rodados com trabalho alheio na árvore (A1-07, IMPORTANTE).
- **Item 4 (NÃO ATENDE pela letra).** 12 das 13 regras de `src/back/domain/regras` citam a linha Excel da aba RN, e as 12 conferem no XLSX oficial. O aviso D1 não cita linha RN (A1-08, decisão do líder).
- **Item 5 (NÃO ATENDE).** A metade do canon corrigido por NL-1.11 confere. Faltam no `TODO.md` as linhas 7 a 12 da INBOX do plano (A1-09).
- **Itens 2 e 10 (PENDENTES da NL-1.7).** A NL-1.7 não está no HEAD `7409319`.
- **Nenhum CRÍTICO** nesta rodada. Há 5 IMPORTANTE (A1-07, A1-09, A1-11, A1-12, A1-13) e 8 COSMÉTICO.
- CI do HEAD verde (run `40278391981`, nove jobs `success`), e o portão A2 verde sem exceção.
- Decisões para o líder, pelo main em AskUserQuestion: A1-11 (botão "Trocar emissor", spec seção 6, dentro da fase 2 da seção 9), A1-08 (letra do item 4 para o aviso D1) e A1-13 (`flatpak run` no host, L-50).

## 2. Veredito item a item

Resumo por item (o detalhe de cada item vem abaixo):

| # | Item da definição | Veredito | Evidência | Severidade do achado |
|---|---|---|---|---|
| 1 | Saída do vermelho e do verde de cada fatia | ATENDE com ressalva | vermelho antes do verde em todas as fatias amostradas; verde limpo na CI do SHA que contém cada fatia (runs `40257570894`, `40260218540`, `40278391981` e outros); logs locais contaminados | IMPORTANTE (A1-07); COSMÉTICO (A1-01, A1-02, A1-03, A1-04, A1-05, A1-06) |
| 2 | `dps validar` com três entradas e `xmllint` | PENDENTE da NL-1.7 | CLI fora do HEAD; `SHA256SUMS` e XLSX conferidos | n/a |
| 3 | Vetores oficiais do CNPJ e normalização DA-1.1 | ATENDE | `tst_cnpj.cpp`, linhas 48 a 84, 97 a 113, 202 a 209 | n/a |
| 4 | Toda regra cita linha RN de 20/10 | NÃO ATENDE (letra) | 12 de 13 regras conferidas no XLSX; `aviso_d1.cpp` sem linha RN | COSMÉTICO (A1-08) |
| 5 | Canon corrigido por NL-1.11 e linhas 1 a 12 da INBOX no `TODO.md` | NÃO ATENDE | canon confere (`2ae04b0`); INBOX só com as linhas 1 a 6 em `TODO.md:14` a `:19` | IMPORTANTE (A1-09); COSMÉTICO (A1-10) |
| 6 | Portão A2 verde sem exceção | ATENDE | CI `40278391981`: `encontrados=167 analisados=167 violacoes=0`; controles b e c provados | n/a |
| 7 | Testes QML, 50 pares, hex vermelho, captura do QA | ATENDE (letra) | `tst_qml_gui` verde em quatro nativos; 50 pares; hex `rc=1`; relatório do QA e md5 congelados | IMPORTANTE (A1-11, A1-12) |
| 8 | ADR-S1 e ADR-S4 aceitos por Narciso | ATENDE | `docs/adr/0002` e `0005`, linha 3, ambos datados 25/10/2026 | n/a |
| 9 | internal-auditor sem CRÍTICO | ATENDE nesta rodada | 0 CRÍTICO nos itens 1 a 8 e 11; NL-1.7 não auditada | n/a |
| 10 | CI verde nas quatro distros | PENDENTE da NL-1.7 | run `40278391981` verde no HEAD; SHAs intermediários cobertos pelos runs seguintes | n/a |
| 11 | DA-1.1 a DA-1.6 no registro | ATENDE | `decisoes_autonomas_onda1.md` linhas 8 a 13 | n/a |
| extra | Fora dos 11 itens | n/a | `flatpak run` no host, registrado na linha 17 do registro | IMPORTANTE (A1-13) |

### Item 1: saída do vermelho e do verde de cada fatia de comportamento

**Veredito: ATENDE com ressalva.** Todo vermelho amostrado vem antes do verde correspondente, e é de asserção ou de configure (A1-06 é a exceção de compilação). Cada fatia tem verde limpo na CI do SHA que a contém. Os logs locais de algumas fatias estão contaminados por trabalho alheio, o que é achado de processo (A1-07).

**Vermelho, ordem e tipo.** Amostra de 13 fatias, com mtime (`stat -c '%y'`) e leitura do conteúdo:

| Fatia | Vermelho (mtime, log) | Verde local (mtime, log) | Tipo do vermelho |
|---|---|---|---|
| NL-1.0b | 23:09, `NL-1.0b-vermelho.log` (6 de 15 falham) | 23:11, `NL-1.0b-verde.log` (15 de 15) | ctest, asserção |
| NL-1.1 | 22:40, `NL-1.1-vermelho.log` | 22:42, `NL-1.1-verde-debug.log` | QTest `FAIL!`, asserção |
| NL-1.2 | 22:46, `NL-1.2-vermelho.log` | 22:47, `NL-1.2-verde-debug.log` | QTest `FAIL!`, asserção |
| NL-1.3 | 23:16, `NL-1.3-vermelho.log` | 23:27, `NL-1.3-verde-debug.log` | QTest `FAIL!` ("Compared values are not the same") |
| NL-1.4 | 00:06, `NL-1.4-vermelho.log` | 00:09, `NL-1.4-verde-debug.log` | QTest `FAIL!` (`cstComDoisDigitosRejeitado`) |
| NL-1.4c | 01:36, `NL-1.4c-vermelho.log` | 01:46, `NL-1.4c-verde-fedora44-debug.log` | ctest, 1 de 66 |
| NL-1.5 | 01:24, `NL-1.5-vermelho.log`; fix 02:17, `NL-1.5-fix-vermelho.log` | 01:26; fix 02:25, `NL-1.5-fix-verde-fedora44-debug.log` | ctest, asserção |
| NL-1.6a | 00:10, `NL-1.6a-vermelho.log`; hash 00:31 | 00:12; hash 00:43 | `configure_rc=1` no ZIP adulterado (`NL-1.6a-vermelho-hash.log`, linha 395) |
| NL-1.6b | 01:12, `NL-1.6b-vermelho.log`; fix 02:16 | 01:14; fix 02:18 | QTest `FAIL!` (`dpsMeiMinimaValidaNoXsd`, `NL-1.6b-vermelho.log` linha 370) |
| NL-1.6c | 01:52, `NL-1.6c-hash-vermelho.log` | 01:55, `NL-1.6c-hash-verde.log` (2 de 2) | ctest, `hash_zip_adulterado_recusado` |
| NL-1.8 | 22:45 (`NL-1.8-vermelho-tentativa1-harness.log`) e 22:46, `NL-1.8-vermelho.log` | 22:54, `NL-1.8-verde-debug-4.log` (9 de 9) | QTest `FAIL!` e ctest |
| NL-1.10 | 22:57, `NL-1.10-vermelho.log` | 23:31, `NL-1.10-verde3.log` (35 de 35) | unittest `assertEqual` (linha 125) |
| NL-1.12 | 22:23, `NL-1.12-vermelho.log` | 22:24, `NL-1.12-verde-debug.log` (2 de 2) | erro de compilação, ver A1-06 |

- A primeira tentativa de vermelho da NL-1.8 (`NL-1.8-vermelho-tentativa1-harness.log`) não é erro de harness. Tem 25 `FAIL!` de asserção, e 19 deles são de `TstValor`, teste de outra fatia (A1-07).
- Nenhum vermelho amostrado é erro de compilação, exceto o da NL-1.12 (A1-06).

**Verde limpo, pela CI do SHA que contém a fatia (L-12 e L-27: o verde do blob commitado).** Os runs foram lidos com `gh run view --log`. Um run cobre todos os commits anteriores a ele na mesma push, então o SHA da fatia é coberto pelo run do SHA seguinte.

| Fatia | SHA da fatia | Run de CI que a contém | Resultado |
|---|---|---|---|
| NL-1.0 | `dd14d94` | `40254725661` (SHA `140aace`) | `success`. Resumo local com 17 `rc=0` nas quatro distros, debug e asan-ubsan (`NL-1.0-resumo.txt`) |
| NL-1.1 | `4c4016c` | `40257570894` (SHA `cd8f1bd`) | 100% de 8 testes nos quatro nativos |
| NL-1.2 | `cd8f1bd` | `40257570894` | 100% de 8 testes nos quatro nativos |
| NL-1.3 | `40d5cd0` | `40260876384` | `success` |
| NL-1.4 e NL-1.6a | `b1eac5b`, `1c6593a` e correções | `40272577164` (SHA `5e82dcd`) | `success` |
| NL-1.4c | `5e82dcd` | `40272577164` | `success` |
| NL-1.5 | `260ab9e` | `40278391981` (SHA `7409319`) | `success` (o run de `260ab9e` está `cancelled`) |
| NL-1.6b | `72b874e` | `40276365173` | `success` |
| NL-1.6c | sem commit próprio | `40272577164` e seguintes | `success` |
| NL-1.8 e NL-1.10 | `25fdc88`, `5ed9ac9` | `40260218540` (SHA `5ed9ac9`) | 100% de 29 testes nos quatro nativos, com `tst_qml_gui` |
| NL-1.12 | `140aace` | `40254725661` | `success` |

**Logs locais contaminados.** Estes verdes locais não provam o commit, porque rodaram com trabalho não commitado de outras fatias na mesma árvore:

- **NL-1.1.** `NL-1.1-verde-debug.log` (22:42) tem `tst_valor` em falha (linha 88), que é teste da NL-1.2. `NL-1.1-verde2-debug.log` (23:06) tem 6 falhas em `tabelas.*` (linhas 132 a 152), da NL-1.10, commitada às 23:39. O verde limpo está no run `40257570894`. A1-01 (COSMÉTICO, registro).
- **NL-1.2.** `NL-1.2-verde-debug.log` (22:47), `NL-1.2-verde-asan.log` (linha 231) e `NL-1.2-verde-mint.log` terminam com `tst_qml_gui` em falha (linha 177 no debug), da NL-1.8, commitada às 23:39. O verde limpo está no run `40257570894`. A1-02 (COSMÉTICO, registro).
- **NL-1.8.** A última suíte completa verde local é `NL-1.8-verde-debug-4.log` (22:54, 9 de 9). Depois, a correção do cabeçalho (`NL-1.8-vermelho-cabecalho.log`, 23:03, 7 falhas de 15, com `tabelas.*`) e `NL-1.8-verde-qml-cabecalho-detalhe.log` (23:04, 1 teste). `NL-1.8-verde-asan-final.log` (23:00) tem 6 falhas. O verde do estado final está no run `40260218540` (SHA `5ed9ac9`). O commit `25fdc88` não tem run próprio, e a cobertura vem do run seguinte. A1-05 (COSMÉTICO, registro).
- **NL-1.0c.** `NL-1.0c-third_party-verde.log` não é verde: falha de compilação em `tests/unit/back/dps/tst_regime.cpp:54` e `tst_tributacao.cpp:90` (linhas 334 a 354), código da NL-1.4, commitada às 00:59. O log é de 00:19. O vermelho da mesma verificação (`NL-1.0c-third_party-vermelho.log`, configure falha) é válido. Decisão deste capítulo: NL-1.0 é costura de build, e o aceite do plano é build limpo nos presets debug e asan-ubsan nas quatro imagens e no CI. Esse aceite está em `NL-1.0-resumo.txt` e nos runs de CI acima. O verde contaminado vira registro (A1-03, COSMÉTICO). O rótulo de `NL-1.0c-mtime-verde.log` e de `NL-1.0c-mtime-vermelho.log` está invertido (A1-04).

**Causa comum (A1-07).** As evidências locais de NL-1.0c, NL-1.1, NL-1.2, NL-1.6b e NL-1.8 foram geradas com trabalho não commitado de outras fatias, no mesmo diretório de build (`/var/tmp/notalivre-build/debug`). O executor da NL-1.6b registrou isso no próprio nome do arquivo (`NL-1.6b-verde-fedora44-debug-com-wip-alheio.log`). Isso contraria a L-12 (o verde precisa ser do estado do commit) e a L-27 (quem verifica lê o blob commitado, nunca a árvore de trabalho).

**Achados do item 1:**

| ID | Severidade | Achado | Evidência | Remediação | Re-teste |
|---|---|---|---|---|---|
| A1-01 | COSMÉTICO | Log local de NL-1.1 contaminado por NL-1.2 e NL-1.10. O verde limpo está no run `40257570894`. | `NL-1.1-verde-debug.log` linha 88; `NL-1.1-verde2-debug.log` linhas 132 a 152 | Anotar o log como contaminado, ou repetir em `git archive 4c4016c` com SHA no log. | Nenhum obrigatório. |
| A1-02 | COSMÉTICO | Log local de NL-1.2 contaminado por `tst_qml_gui` da NL-1.8. O verde limpo está no run `40257570894`. | `NL-1.2-verde-debug.log` linha 177 | Anotar como contaminado, ou repetir em `git archive cd8f1bd`. | Nenhum obrigatório. |
| A1-03 | COSMÉTICO | Verde do gancho de `third_party` em NL-1.0c contaminado. O aceite de NL-1.0 está em `NL-1.0-resumo.txt` e na CI. | `NL-1.0c-third_party-verde.log` linhas 334 a 354; commit `b1eac5b` (00:59) | Anotar o log como contaminado. | Nenhum obrigatório. |
| A1-04 | COSMÉTICO | Rótulo invertido. `NL-1.0c-mtime-verde.log` contém `#error SABOTAGEM_NL10C_MTIME` (linha 149), que é vermelho de sabotagem. `NL-1.0c-mtime-vermelho.log` é o verde de 35 de 35. | Os dois arquivos | Renomear no registro, sem apagar. | Nenhum; é registro. |
| A1-05 | COSMÉTICO | Commit `25fdc88` sem run de CI próprio. Coberto pelo run `40260218540` (SHA `5ed9ac9`, com 29 testes verdes). Log local `NL-1.8-verde-asan-final.log` com 6 falhas, de estado intermediário. | `NL-1.8-verde-asan-final.log`; run `40260218540` | Anotar o estado intermediário no registro. | Nenhum obrigatório. |
| A1-06 | COSMÉTICO | O vermelho da NL-1.12 é de compilação (cabeçalho `nl/base/erro.h` ainda não existe). Não se viu nenhuma asserção do tipo `Erro` falhar. | `NL-1.12-vermelho.log` linha 62 | Registrar a exceção e a L-35 no dossiê. Vermelho de asserção para `erroGuardaCodigoCampoEMensagem` é recomendado para o futuro. | Nenhum obrigatório. |
| A1-07 | IMPORTANTE | Árvore e build compartilhados entre executores. Evidência local com trabalho alheio em cinco fatias. | `/var/tmp/notalivre-build/debug` nos logs de 1.0c, 1.1, 1.2, 1.6b e 1.8; `NL-1.6b-verde-fedora44-debug-com-wip-alheio.log` | Processo: um diretório de build por fatia e execução sobre `git archive`, conforme L-12 e L-27. | Próximo log de verde indica o SHA e não contém teste de outra fatia. |

### Item 2: `notalivre dps validar` com as três entradas e o `xmllint`

**Veredito: PENDENTE da NL-1.7 (sem veredito, por ordem de serviço).** A NL-1.7 não está no HEAD `7409319`: `apps/cli/comando_dps_validar.cpp`, `apps/cli/argumentos_dps_validar.cpp`, `src/mid/composicao/dps_validar.cpp` e `tests/e2e/dps_validar` não existem no commit (`git cat-file -e`). O trabalho dela está em correção na árvore de trabalho, não auditado.

Registro do que o HEAD já contém e que este item usa, sem veredito:

- `third_party/nfse-xsd/prodrest-20261020/SHA256SUMS` no HEAD traz a URL do ZIP, o SHA-256 do ZIP `25fdcecc651a67a216496ab10d7566631e6ed10f721e37d62c7845dadb0ea267` e os dez SHA-256 dos arquivos `.xsd`. Os dez pares (hash e nome) são idênticos aos da seção 5 de `docs/pesquisa/onda1_leiaute.md` (`diff` de listas ordenadas, vazio).
- SHA-256 do XLSX do Anexo de 20/10 recalculado nesta auditoria: `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8`. Confere com o valor de `docs/pesquisa/onda1_leiaute.md` (seção 0, linha 23), na cópia de `/var/tmp/notalivre-nl111/dl/`.
- As entradas de referência existem no HEAD: `tests/fixtures/dps/valida_dps_mei_minima.xml`, `valida_dps_simples_ibscbs.xml` e `valida_dps_tomador_cpf_sem_endereco.xml`, mais a entrada sem campo obrigatório, `tests/fixtures/xsd/invalida_campo_obrigatorio_ausente.xml`. Não foram executadas nesta auditoria, e nenhuma delas é a saída da CLI.

O que fica para o fechamento da NL-1.7: a geração pela CLI, o `xmllint` sobre os três XML gerados, a comparação do `SHA256SUMS` com `onda1_leiaute.md` (já igual no HEAD, ver acima) e a reprovação da entrada sem campo obrigatório pela CLI. Sem a CLI no HEAD, esses passos não podem ser conferidos agora.

### Item 3: vetores oficiais do CNPJ e normalização de DA-1.1 nos testes de NL-1.1

**Veredito: ATENDE.** Arquivo: `tests/unit/back/identificacao/tst_cnpj.cpp`, lido no blob do HEAD. Os quatro vetores oficiais estão nas funções de teste, com `QVERIFY(cnpj.has_value())` e `QCOMPARE` sobre o texto normalizado:

| Vetor (Receita e Serpro) | Função | Linhas | Conta |
|---|---|---|---|
| `12ABC34501DE35` | `cnpjAlfanumericoValidoAceito` | 57 a 65 | DV 35 |
| `AA345678000A29` | `cnpjLetraNaOrdemAceito` | 78 a 84 | DV 29 |
| `12345678000A08` | `cnpjAlfanumericoRestoUmDaDvZero` | 68 a 75 | resto 0 ou 1, DV 08 |
| `12345678000195` | `cnpjNumericoValidoAceito` | 48 a 54 | numérico |

Normalização de DA-1.1 (maiúscula, máscara de ponto, barra, hífen e espaço): `normalizarRemoveSoSeparadoresPermitidos` (linhas 108 a 113, inclusive `12.ABC/345-01 DE35`), `cnpjMinusculoNormalizadoParaMaiusculo` (97 a 105), `cnpjComMascaraNormalizado` (87 a 94) e a tabela `Caso` das linhas 202 a 209 (minúsculas e máscaras nos dois CNPJs). Negativos: `cnpjDvErradoRejeitado` (DV `12ABC34501DE36`, linha 168), `cnpjLetraNoDvRejeitada` e `cnpjMaisDeQuatorzeRejeitado`.

Ressalva de escopo: a Review Focus da linha 1 do plano cita o teste `normalizacaoIgualNasTresEntradas`, que fica em NL-6.5, e não entra nesta conta. Os vetores de NL-1.1, que são o insumo dele, estão aqui.

Nenhum teste foi executado por esta auditoria. O verde limpo de NL-1.1 está no run `40257570894` (item 1).

### Item 4: toda regra de `src/back/domain/regras` cita a linha da aba RN do Anexo de 20/10

**Veredito: NÃO ATENDE pela letra (1 arquivo sem citação de linha RN).** O conjunto, com 17 arquivos no HEAD (`git ls-tree -r`; lista gravada em `/var/tmp/notalivre-logs/aud1_regras_head.txt`), foi enumerado por inteiro. A conferência foi feita no XLSX oficial com leitor da biblioteca padrão (`zipfile` e `xml.etree`, `python3 -I`), somente leitura, sobre o arquivo de SHA-256 `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8`. Aba `RN DPS_NFS-e 1.01`:

| Arquivo (HEAD) | Citação no código | Linha Excel citada | Coluna H da linha no XLSX | Veredito |
|---|---|---|---|---|
| `regra_e0202.cpp:30` | E0202 | 257 | E0202 | confere |
| `regra_e0204.cpp:37` | E0204 | 258 | E0204 | confere |
| `regra_e0237.cpp:33` | E0237 | 277 | E0237 (coluna A da linha é `#REF!`, a regra confere pela coluna H) | confere |
| `regra_e0241.cpp:39` | E0241 | 278 | E0241 | confere |
| `regra_e0246.cpp:36` | E0246 | 284 | E0246 (o `Obs 2` do texto não foi comparado palavra a palavra) | confere |
| `regra_e0302.cpp:26` | E0302 | 333 | E0302 | confere |
| `regra_e0390.cpp:25` | E0390 | 380 | E0390 | confere |
| `regra_e0532.cpp:32` | E0532 | 484 | E0532 | confere |
| `regra_e0583.cpp:28` | E0583 | 513 | E0583 | confere |
| `regra_e0710.cpp:30` | E0710 | 565 | E0710 | confere |
| `regra_e0850.cpp:24` | E0850 | 566 | E0850 | confere |
| `regra_e0903.cpp:31` | E0903 | 574 | E0903 | confere |
| `aviso_d1.cpp:3` | aviso D1, sem código E | "ponto L10" do plano, sem linha da aba RN | não se aplica | NÃO ATENDE pela letra (A1-08) |

Arquivos sem regra fiscal, que não entram na letra do item: `CMakeLists.txt`, `regra_aviso.cpp` e `regra_local.cpp` (só o destrutor virtual, para ancorar o vtable), e `validador_local.cpp` (agregador que aplica todas as regras e avisos).

Achado A1-08 (COSMÉTICO por risco, mas bloqueia a letra do item 4). `aviso_d1.cpp` não é regra do Anexo: é o aviso D1 da decisão do líder L10 ("Leiaute, com aviso do site"), e o ponto L10 mora no plano. A letra do item exige que toda regra cite a linha RN. Remediação: ou o aviso cita o ponto L10 e a origem do site como fonte, e o item 4 é reescrito pelo líder, ou ganha uma linha de referência no próprio cabeçalho. Não há risco fiscal: é só aviso, nunca erro.

Confirmação de DA-1.29 (fora do item 4, mas usada aqui): E0905 e E0906 não estão em `regras/`. Estão em `src/back/domain/dps/ibscbs.cpp:31` e `:35` (NL-1.4). O XLSX confirma as linhas 576 (E0905) e 577 (E0906), e não as linhas 709 e 710 do Anexo VI apontadas pelo revisor. A decisão DA-1.29 (a) está correta pelo XLSX.

Confirmação de DA-1.22: `RegraE1302` não existe mais. O único `E1302` no HEAD é um comentário em `tests/unit/back/regras/tst_validador_local.cpp:27`, que registra a retirada.

### Item 5: canon corrigido por NL-1.11 e linhas 1 a 12 da INBOX no `TODO.md`

**Veredito: NÃO ATENDE (IMPORTANTE), pela metade da INBOX.**

**Metade 1, canon corrigido por NL-1.11: confere.** Commit `2ae04b0` (NL-1.11) toca os seis arquivos esperados (`git show --stat`: `TODO.md`, `docs/design/spec-visual.md`, `docs/pesquisa/campos_emissor_web.md`, `docs/pesquisa/onda1_leiaute.md`, o plano e a spec de design). Medido no blob do HEAD:

- `docs/design/spec-visual.md` linhas 42, 135, 194 e 199: "2000 caracteres".
- `docs/pesquisa/campos_emissor_web.md` linhas 29, 98, 403 e 437: o texto distingue o Anexo de 09/02 e o de 20/10 (linha 98: `C/2000`, linha Excel 201).
- `docs/pesquisa/onda1_leiaute.md` seção 0: o SHA-256 do XLSX `72b05981...` está presente.
- O plano não diz mais que o DV está em aberto nas linhas 1 e 13 (a única ocorrência de "EM ABERTO" perto de CNPJ é a própria instrução da NL-1.11, linha 428).

**Metade 2, linhas 1 a 12 da INBOX do plano no `TODO.md`: não confere.** A INBOX do plano (seção da Onda 1, `docs/superpowers/plans/2026-10-25-notalivre-plano.md`, linhas 273 a 284) tem 12 linhas. O `TODO.md` do HEAD, seção `## INBOX` (linhas 7 a 43), tem só as linhas 1 a 6, com o texto ajustado ao plano:

- Presentes: linhas 1 a 6 da INBOX do plano, em `TODO.md:14` a `TODO.md:19`, com "Fonte: plano, Onda 1, INBOX item N".
- Ausentes: linhas 7 a 12 do plano, que são (7) intermediário `interm`, com `vReceb`, e a E0262; (8) código de tributação municipal `cTribMun`; (9) tributos federais `valores/trib/tribFed`; (10) informações complementares `xInfComp`, `cIntContrib` e CAEPF; (11) descontos `vDescCondIncond` e ISSQN especial; (12) IBS/CBS especial (`indFinal`, `tpEnteGov`, `dest`, `imovel`, `gIBSCBS`). Busca por termos e por "Capitolino" no `TODO.md` não acha nenhum deles. A única menção a `tpEnteGov` é a linha `TODO.md:42`, derivada de DA-1.26, que é só uma parte do item 12.

Achado A1-09 (IMPORTANTE). Seis decisões de produto para o Capitolino não estão na tabela de pendências. L-63 exige que todo trabalho descoberto vá à INBOX imediatamente. Remediação: incluir as linhas 7 a 12 do plano em `TODO.md`, na seção INBOX, com a fonte citada, sem ordenar.

Achado A1-10 (COSMÉTICO). As referências de linha das INBOX de `TODO.md` divergem das do plano, que são as linhas Excel decididas em DA-1.7. `TODO.md:14` diz "aba 6 linha 385" (plano: linha Excel 386); `TODO.md:15` diz 255 (plano: 256); `TODO.md:16` diz 366 (plano: 367 e 373); `TODO.md:17` diz "aba RN linhas 166, 203 e 482" (plano: 167, 206, 485). Os números do `TODO.md` são de outra numeração, que DA-1.7 rejeitou. Remediação: alinhar a `TODO.md` ao plano.

### Item 6: portão A2 verde, sem exceção

**Veredito: ATENDE.**

- O portão de camadas é `tools/portoes/camadas.py` com o autoteste `tools/portoes/autoteste_camadas.sh`. Nenhum dos dois mudou entre `main` e `onda-1` (`git diff main..onda-1 -- tools/portoes/camadas.py` vazio). `git grep -i "excec|allow|ignora"` em `camadas.py` no HEAD não acha nenhuma lista de exceção.
- Os arquivos de exceção que mudaram em `onda-1` são outros: `tools/portoes/spdx_excecoes.txt` (+3 linhas, arquivos JSON sem comentário, com justificativa escrita em cada linha) e `tools/portoes/travessao_config.json` (`piso_ui_ligado` de `false` para `true`, o que o próprio arquivo previa para a NL-1.8). Nenhum deles é o gate A2.
- CI do HEAD (run `40278391981`, `headSha` `7409319`, job `camadas (portão A2 e autoteste)`, log lido com `gh run view --log`): `encontrados=167 analisados=167 violacoes=0` (rc=0). No autoteste, controle a rc=0, controle b rc=1 com `VIOLACAO src/back/camada.cpp:8` (sabotagem: `back` incluindo `nl/mid/camada.h`), controle c rc=1 com `VARREDURA VAZIA` e `encontrados=0 analisados=0`, o que prova que zero reprova.
- Job `mutação dos portões` no mesmo run: `portao=a2 rc=1 motivo=ok` no SHA `7409319`, além de gitleaks, travessão, spdx, hex e flatpak611 com `rc=1 motivo=ok`.

### Item 7: testes QML offscreen verdes, 50 pares de contraste, portão de hex visto vermelho, relatório de captura do qa-engineer

**Veredito: ATENDE pela letra, com achados IMPORTANTES (A1-11, A1-12) no conteúdo da captura.**

- **Testes QML verdes offscreen.** `tst_qml_gui` (ctest 46 de 66) passou nos quatro nativos do CI do HEAD (run `40278391981`, logs de `nativo (fedora44)`, `(manjaro)`, `(mint22.3)` e `(ubuntu26.04)`, linhas 2133, 3996, 5338 e 8483). `tests/qml/CMakeLists.txt` declara `tst_qml_gui` como executável único, com `QT_QPA_PLATFORM=offscreen` (comentário na linha 3).
- **Os 50 pares.** `tests/qml/dados/pares_contraste.json` tem 50 pares (22 com `publicada` e 28 derivados, `python3 -I` sobre o blob do HEAD). `tests/qml/tst_Tema.qml:128` (`test_contrasteDos50Pares`) faz `compare(dados.pares.length, 50)` e reprova se a razão for menor que `minimo`. Portanto o teste confere a contagem de 50 e não de 22. `QUICK_TEST_MAIN_WITH_SETUP` (`tests/qml/tst_main.cpp:46`) roda os `tst_*.qml` do diretório de testes, então o passe de `tst_qml_gui` cobre o teste (o log do CI não mostra linha separada por teste).
- **Portão de hex visto vermelho.** No CI do HEAD: `portao=hex rc=1 motivo=ok sha=7409319...` (log da `mutação dos portões`, linha 2482). Local: `NL-1.8-portao-hex-sabotagem.log` com `violacao: apps/gui/qml/Casca.qml:16 hex=#FF0000`, `encontrados=8 analisados=8 violacoes=1`.
- **Relatório de captura do qa-engineer.** `/var/tmp/notalivre-logs/NL-1.8-relatorio-captura.md` (qa-engineer, L-13), com 19 itens conferidos contra `docs/design/spec-visual.md` e o mockup `04-telas-principais.html`. Os md5 de referência batem com os valores atuais no HEAD (`spec-visual.md` `99ec4383...`, mockup `dc5b6c6a...`). Congelamento: `NL-1.8-captura/md5-inicio.txt` e `md5-fim.txt` são idênticos (4 PNGs). `NL-1.8-captura.rc` tem `rc=0`. Verifiquei por imagem `janela-emissao-claro.png`: o cabeçalho tem "Atualizar tabelas" e "Emissor: Ateliê Cacto Verde MEI", e não tem "Trocar emissor".

**Achados do item 7:**

| ID | Severidade | Achado | Evidência | Remediação | Re-teste |
|---|---|---|---|---|---|
| A1-11 | IMPORTANTE | Item 17 do relatório, "Trocar emissor" à direita do cabeçalho, está em FALHA. A spec seção 6 (`docs/design/spec-visual.md:180`) define o cabeçalho com esse link. A fase 2 da seção 9 (`spec-visual.md:307`) põe "cabeçalho, emissor atual" no escopo de shell, e o plano da NL-1.8 (linha 410) diz que a fatia cobre as fases 1 e 2. Não é fase posterior. A ausência foi adiada pela INBOX (`TODO.md:24`) sem decisão do líder registrada. | `NL-1.8-relatorio-captura.md`, item 17; `janela-emissao-claro.png` (cabeçalho); spec seções 6 e 9 | Levar ao líder: implementar agora ou alterar a spec por decisão dele. Não cabe ao agente decidir o adiamento (L-01). | Captura nova com o botão, ou decisão registrada do líder. |
| A1-12 | IMPORTANTE | A captura depende de `tests/qml/captura/tst_CapturaJanela.qml`, que não está no HEAD (a listagem de `tests/qml` no commit `7409319` não a inclui). O relatório diz "não commitado". Evidência não reproduzível a partir do repositório. | `NL-1.8-relatorio-captura.md`, seção "Artefatos" | Commitar o teste de captura em commit próprio, com ID, ou registrar a exceção. | Reprodução da captura a partir do SHA, md5 igual. |

Ressalva sobre os itens 18 e 19 do relatório: foco visível e Orca estão como NÃO MEDIDO. Não são obrigação do item 7 (a letra pede o relatório, que existe), mas ficam registrados.

### Item 8: ADR-S1 e ADR-S4 aceitos por Narciso

**Veredito: ATENDE pela letra.**

- ADR-S1 (`docs/adr/0002-custodia-do-a1.md`, linha 3): "ACEITO por Narciso (CISO) em 25/10/2026 (N2)".
- ADR-S4 (`docs/adr/0005-logs-sem-pii.md`, linha 3): "ACEITO por Narciso (CISO) em 25/10/2026 (N3)".
- Confirmação em `docs/seguranca/modelo-de-ameacas.md`, linha 185: "ADR-S1, ADR-S4 e ADR-S5 aceitos".
- Ressalva, sem achado: o registro `decisoes_autonomas_onda1.md` (linha 18) põe o veredito do CISO como decisão autônoma (L-15), "a ratificar pelo líder ao sair do modo". A ratificação do líder está pendente e não é ato deste capítulo.

### Item 9: internal-auditor sem CRÍTICO

**Veredito: ATENDE nesta rodada (0 CRÍTICO nos itens 1 a 8 e 11).** A NL-1.7 e os itens 2 e 10 não foram auditados (pendentes), então a conta final do item 9 depende deles. Este capítulo fica como está até a NL-1.7 ser reauditada.

### Item 10: CI verde nas quatro distros

**PENDENTE da NL-1.7 (sem veredito, por ordem de serviço).** Registro sem veredito:

- Run `40278391981`, SHA `7409319`, `success`, nove jobs `success` (nativo fedora44, ubuntu26.04, manjaro e mint22.3; higiene; camadas; flatpak; smoke; mutação).
- Commits intermediários sem run próprio: `4c4016c`, `25fdc88`, `b1eac5b`, `dd14d94` e `06a7f45`. Estão cobertos pelos runs dos SHAs seguintes (item 1). Isso é consequência do push em lote, e não foi verificado além disso (INFERÊNCIA: o GitHub gera run só no topo de cada push).
- `260ab9e` e `4c9f9ed` estão `cancelled`, não `failure`. `260ab9e` é coberto pelo run `40278391981`.
- `1c6593a`, `63d8080`, `38c5885`, `8b8508b`, `dc236ba` e `66539de` estão `failure`, e foram corrigidos pelos commits seguintes (`5e82dcd`, `da73c5b`, `72b874e` verdes).

### Item 11: decisões DA-1.1 a DA-1.6 no registro

**Veredito: ATENDE.** `/var/tmp/notalivre-logs/decisoes_autonomas_onda1.md`, linhas 8 a 13, com cada decisão datada (25/10/26 21:50:37 e 22:10:23), pergunta, opções, escolha de quem decide e porquê:

- DA-1.1 (linha 8): normalização do CNPJ.
- DA-1.2 (linha 9): limite de `xDescServ`, 2000.
- DA-1.3 (linha 10): E9996 e regras do Anexo antigo.
- DA-1.4 (linha 11): referência da assinatura, ponto aberto para a Onda 2.
- DA-1.5 (linha 12): campos novos e removidos.
- DA-1.6 (linha 13): levas e paralelismo da Onda 1.

Observação fora dos 11 itens, registrada no próprio arquivo, linha 17 (22:20:53): "medição do SDK rodou com flatpak run no host (L-50), sem janela". A decisão foi refazer em contêiner. Ver A1-13.

### Achado fora dos 11 itens

| ID | Severidade | Achado | Evidência | Remediação | Re-teste |
|---|---|---|---|---|---|
| A1-13 | IMPORTANTE | Execução de `flatpak run` no host, na sessão do líder, contra a L-50 (teste ou binário rodando na máquina viva). O registro admite e manda refazer em contêiner. A refação aparece em `NL-1.0-sdk611-*.log` (22:22 a 22:31). | `decisoes_autonomas_onda1.md`, linha 17; `NL-1.0-sdk611-conteiner-tentativa-flatpak-run.log` | Levar ao líder: ratificar ou registrar a quebra da L-50. Garantir que o host não é mais usado para isso. | Nenhum log de `flatpak run` no host em `/var/tmp/notalivre-logs`. |

## 3. Método

- Comandos read-only: `git log`, `git show <SHA>:<arquivo>`, `git ls-remote`, `gh run list`, `gh run view --log`, `stat`, `grep` sobre os logs.
- Nenhum build, nenhum teste, nenhum contêiner e nenhuma sabotagem foi executado por esta auditoria. Os vereditos vêm dos logs, dos runs de CI e dos blobs.
- Vermelho contra verde: comparação de mtime (`stat -c '%Y %n'`), leitura do conteúdo do vermelho (asserção de QTest, `assertEqual` ou `ctest` contra erro de compilação) e, para o verde limpo, o run de CI do SHA que contém a fatia.

## 4. Achados

### 4.1 CRÍTICO

Nenhum nesta rodada.

### 4.2 IMPORTANTE

| ID | Item | Achado | Evidência principal |
|---|---|---|---|
| A1-07 | 1 | Árvore e diretório de build compartilhados entre executores; evidência local com trabalho alheio em cinco fatias (processo, contra L-12 e L-27) | `/var/tmp/notalivre-build/debug` nos logs; `NL-1.6b-verde-fedora44-debug-com-wip-alheio.log` |
| A1-09 | 5 | Linhas 7 a 12 da INBOX do plano ausentes do `TODO.md` | `TODO.md:7` a `:43`; plano linhas 273 a 284 |
| A1-11 | 7 | Botão "Trocar emissor" ausente no cabeçalho, exigido pela spec seção 6 e pela fase 2 da seção 9; adiado pela INBOX sem decisão do líder | `NL-1.8-relatorio-captura.md`, item 17; `janela-emissao-claro.png`; `docs/design/spec-visual.md:180` e `:307` |
| A1-12 | 7 | Captura depende de teste não commitado (`tests/qml/captura/tst_CapturaJanela.qml`) | `NL-1.8-relatorio-captura.md`, seção "Artefatos" |
| A1-13 | fora | `flatpak run` no host na sessão do líder (L-50), refeito em contêiner | `decisoes_autonomas_onda1.md`, linha 17 |

### 4.3 COSMÉTICO

| ID | Item | Achado | Evidência principal |
|---|---|---|---|
| A1-01 | 1 | Log local de NL-1.1 contaminado; verde limpo no run `40257570894` | `NL-1.1-verde-debug.log:88`; `NL-1.1-verde2-debug.log:132` a `152` |
| A1-02 | 1 | Log local de NL-1.2 contaminado; verde limpo no run `40257570894` | `NL-1.2-verde-debug.log:177` |
| A1-03 | 1 | Verde do gancho de `third_party` (NL-1.0c) contaminado; aceite de NL-1.0 em `NL-1.0-resumo.txt` e na CI | `NL-1.0c-third_party-verde.log:334` a `354` |
| A1-04 | 1 | Rótulos invertidos em `NL-1.0c-mtime-verde.log` e `NL-1.0c-mtime-vermelho.log` | linha 149 do primeiro (`#error SABOTAGEM_NL10C_MTIME`) |
| A1-05 | 1 | Commit `25fdc88` sem run próprio; coberto pelo run `40260218540`. Log intermediário com 6 falhas | `NL-1.8-verde-asan-final.log`; run `40260218540` |
| A1-06 | 1 | Vermelho da NL-1.12 é de compilação (`nl/base/erro.h` inexistente); nenhuma asserção do tipo `Erro` foi vista falhando | `NL-1.12-vermelho.log:62` |
| A1-08 | 4 | `aviso_d1.cpp` não cita linha da aba RN; cita o ponto L10 do plano | `src/back/domain/regras/aviso_d1.cpp:3` |
| A1-10 | 5 | Referências de linha das INBOX do `TODO.md` na numeração antiga, contrária a DA-1.7 (385 contra 386, 255 contra 256, 166 contra 167, etc.) | `TODO.md:14` a `:17` |

## 5. Varreduras

| Varredura | Resultado |
|---|---|
| CI do HEAD, higiene: gitleaks | `encontrados=386 analisados=386 segredos=0 gitleaks_rc=0` |
| CI do HEAD, higiene: SPDX | `encontrados=386 analisados=386 codigo=314 violacoes=0 excecoes=5` |
| CI do HEAD, camadas | `encontrados=167 analisados=167 violacoes=0`; autoteste com varredura vazia reprovando (controle c) |
| CI do HEAD, mutação | `portao=a2`, `gitleaks`, `travessao`, `spdx`, `hex` e `flatpak611` com `rc=1 motivo=ok`, `sha=7409319` |
| CI de `cd8f1bd` e `5ed9ac9` (item 1) | 100% dos testes nos quatro nativos (8 e 29 testes) |
| Hash do ZIP e dos dez `.xsd` | `SHA256SUMS` idêntico à seção 5 de `onda1_leiaute.md` (`diff` vazio) |
| Hash do XLSX do Anexo de 20/10 | `72b05981c8882cdd807f3acf901b669465360f9cf1b9d9e235ac4363dacfb2d8` |
| Linhas Excel das 12 regras E | conferidas no XLSX pelo código na coluna H (tabela do item 4) |
| Travessão (U+2014) e meia-risca (U+2013) neste capítulo | zero (varredura de encerramento, seção 7) |

## 6. Conformidade observada (sem achado)

- L-63: todas as linhas NL-1.0 a NL-1.12 de `TODO.md` estão com status "Pendente verificação", exceto NL-1.7, que está "Pendente". A coluna Estado Auditado está sem marcador em todas.
- L-24: 22 dos 25 commits de `main..onda-1` citam o ID `NL-1.x`. Os três sem ID são de documento e não são fatias: `e646fad` (pesquisa de abertura), `06a7f45` (INBOX) e `e392ce6` (parecer K2).
- Portão A2 e sabotagens do CI do HEAD: vermelho provado (item 6).
- Actions e permissões: não auditadas nesta rodada.

## 7. Limites desta auditoria

- Nenhum build, teste, contêiner, sabotagem ou snapshot foi executado. Os vereditos vêm de logs, de runs de CI lidos com `gh run view --log`, de blobs do HEAD e de leituras de arquivos.
- A árvore de trabalho está suja com a NL-1.7 em correção. Nada dela foi lido para o veredito.
- A conferência do XLSX foi feita com leitor da biblioteca padrão, somente leitura, sobre o SHA-256 verificado. Não foi comparado o texto de cada linha (só o código na coluna H e a linha citada).
- Os logs de `/var/tmp/notalivre-logs/` são evidência de terceiros (executores). A identidade dos agentes que os geraram não é verificável pelo log.
- Varredura de travessão e meia-risca (U+2014 e U+2013) e de emoji feita sobre o arquivo final: zero ocorrências de travessão e meia-risca. Não há emoji. Os símbolos de status do `TODO.md` foram escritos por extenso.

## 8. Plano de remediação e re-teste

| Ordem | Ação | Achados | Resultado esperado |
|---|---|---|---|
| R1 | Anotar nos logs locais de NL-1.0c, NL-1.1, NL-1.2 e NL-1.8 que são contaminados, e apontar o run de CI que é o verde limpo | A1-01, A1-02, A1-03, A1-05 | Registro sem ambiguidade |
| R2 | Registrar a NL-1.0c com rótulos corretos | A1-04 | Registro sem ambiguidade |
| R3 | Incluir as linhas 7 a 12 da INBOX no `TODO.md` e alinhar as referências de linha a DA-1.7 (main, sem ordenar) | A1-09, A1-10 | Seis linhas a mais na INBOX, com fonte |
| R4 | Líder decide a letra do item 4 para o aviso D1 | A1-08 | Item 4 reescrito ou aviso com linha RN |
| R5 | Líder decide o botão "Trocar emissor" (implementar ou mudar a spec) | A1-11 | Captura nova com o botão, ou decisão registrada |
| R6 | Commitar o teste de captura em commit próprio com ID | A1-12 | Captura reproduzível do SHA |
| R7 | Líder ratifica ou registra a quebra da L-50 | A1-13 | Decisão registrada |
| R8 | Processo: um diretório de build por fatia e execução sobre `git archive`, conforme L-12 e L-27 | A1-07 | Nenhum log com trabalho alheio |
| R9 | Auditar a NL-1.7 e fechar os itens 2, 9 e 10 | itens 2, 9 e 10 | Veredito dos itens 2, 9 e 10 |

**Estado Auditado sugerido ao main** (o main grava no `TODO.md`, com o marcador de aviso do schema): NL-1.0, NL-1.1, NL-1.2, NL-1.5, NL-1.8 e NL-1.12 com aviso, pelos achados acima. As demais NL-1.x (NL-1.3, NL-1.4, NL-1.6, NL-1.9, NL-1.10 e NL-1.11) e a NL-1.7 ficam como não auditadas, porque esta rodada não as cobriu além da prova de vermelho e verde.

**Parecer de prontidão para auditor externo:** não pronto. Faltam a NL-1.7 (itens 2 e 10), a metade da INBOX (item 5) e a letra do item 4. Os achados A1-11 e A1-12 pedem decisão do líder ou correção antes de a captura servir de evidência externa.
