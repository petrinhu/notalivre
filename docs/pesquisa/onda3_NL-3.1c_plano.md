<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa -->

# Plano da fatia NL-3.1c: cópia do banco e prova de restauração num banco temporário

Autor: Caetano (CTO), 28/11/2026, 18:39 UTC. Worktree `worktrees/NL-3.1c`, branch `fatia/NL-3.1c`, saída da `onda-3` em `46e38eb`, lida sem editar código, sem commit, sem build, sem CTest, sem contêiner e sem o `emitir_teste.sh` (NF-04: o CI é o portão).

Frase-guarda (L-21), verbatim:

> Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição.

Item da fatia (`TODO.md:277`), verbatim: "Cópia e restauração: arquivo cifrado por campo, gerado pelo próprio app, sem chave nova, em pasta privada, restaurado num banco temporário. [PORTA] P4." No plano do projeto (`docs/superpowers/plans/2026-10-25-notalivre-plano.md:698`): "NL-3.1c cópia: arquivo cifrado por campo, gerado pelo próprio app, sem chave nova, em pasta privada, restaurado num banco temporário, o que prova a restauração." Depende da NL-3.1a; a NL-3.1d depende desta (`TODO.md:278`); o fechamento da Onda 3 pede "cópia provada por restauração" nas duas famílias (plano, `:724`).

**Leis que disparam.** NF-02 (código novo em `src/foundation/sql` e `src/mid/persistencia`, que a lei já lista; nenhum motor da Foundation inclui outro, NL-I.093), NF-03 (nomes e `std::expected`), NF-04 (CI seletivo; a fatia muda `CMakeLists`, então roda tudo), NF-05 (nada novo a baixar: o CBOR é do QtCore), NF-07 (um pesado por vez, até 4 agentes; e os registros de 04/11 que esta fatia cita estão em `/var/tmp`, seção 0.2), NF-08 e NF-09 (commit, push e conferência), NF-10 (SPDX e autoria), NF-12 (a fatia toca segurança e privacidade: a mutação vai ao líder, P2), NF-17 (seção 3), NF-18 (texto), NF-20 (merge só com APROVA), NF-21 (Status), NF-22 (a NL-3.8 continua depois da NL-3.1d), NF-25 (pesquisa na seção 0.3), NF-29 (a cópia mora na pasta de dados do app, onde a ADR-0003 põe o arquivo S; o código de recuperação nunca vai lá), NF-32 (átomos), NF-33 (executor, revisor e orquestrador distintos; TDD), NF-34 (c) (todo SQL literal, com parâmetro vinculado; nenhum SQL vem do arquivo).

**Leis que não disparam, conferidas.** NF-06 (nenhuma validação de certificado nem revogação); NF-27 (cadastro, admin e quiz não mudam); NF-28 (a remoção de cofre não muda, mas um cofre removido continua nas cópias até elas vencerem, como o tomador apagado: item (f) da seção 10); NF-30 (a fatia recebe a conexão pronta, não lê a senha do banco e não muda a conferência do servidor).

Legenda: **FATO** (lido ou medido, com fonte), **INFERÊNCIA** (conclusão minha, com a base), **NÃO MEDIDO**, **DECISÃO DO CTO** (técnica e reversível, DA-3.1c.N na seção 7), **REDAÇÃO DO CISO** e **REDAÇÃO DO CTO** (texto de registro, não verbatim do líder, L-07).

Para a thread principal: em todo despacho desta fatia, ponha no prompt o caminho dos manuais do plugin bigtech (CONTRACT.md, TESTES.md, DEPLOY_CHECKLIST.md e AUDITORIAS.md), o caminho deste plano e a frase-guarda verbatim; o subagente não herda o contexto da sessão. Os cinco registros de 04/11/2026 da seção 0.2 estão em `/var/tmp/notalivre-logs/` e somem em 30 dias: copie-os para `~/.local/share/notalivre/` (NF-07) antes do passo 0.

---

## 0. O que foi lido, medido e pesquisado

### 0.1 Código e canon

Lidos: `CLAUDE.md` e `GODS_LAWS.md` do worktree; o plano do projeto (P4 em `:156`, NL-3.1 em `:698`, NL-3.6 em `:703`, fechamento da Onda 3 em `:716-733`, tabela 6.2 com L3 em `:982`, P4 em `:1004` e L11 em `:1005-1011`); a spec (4.4, 4.15 e 4.16); `docs/adr/0003-envelope-e-hierarquia-de-chaves.md`, `0005-logs-sem-pii.md` e `0008`; `docs/seguranca/modelo-de-ameacas.md`; `docs/pesquisa/onda3_banco_cripto.md` e `onda3_nl-i182.md`; o modelo de forma `docs/pesquisa/onda3_NL-3.M_plano.md`; `TESTES.md` e `AUDITORIAS.md`; em `src/foundation/sql`, os cabeçalhos públicos e `executor.cpp`, `leitura.cpp`, `transacao.cpp`, `opcoes_sessao.cpp`, `migracao_aplicacao.cpp`, `migracao_catalogo.cpp` e `trava_migracao.cpp`; `src/foundation/cripto/include/nl/foundation/cripto/aead.h`; `src/mid/persistencia` e o cabeçalho público dela; `src/mid/composicao/resposta_crua.cpp` e `emissao_cli.cpp`; os testes `tests/integracao/banco/*` e `tests/unit/foundation/sql/*`; `tools/conteiner/banco/sql_root.sh`, `credencial.sh`, `tools/conteiner/integracao_banco.sh` e `matriz_banco.txt`; `tools/portoes/sql_seguro.py` e `camadas.py`; o job `banco` do `.github/workflows/ci.yml`; os pisos.

Medições por `git grep` e `ls` no worktree em `46e38eb` (FATO):

1. **Nenhuma chave de dados existe no código.** A busca por `dek`, `k_campo` e "segredo da instalação" em `src/` e `apps/` não acha implementação; a primitiva AES-256-GCM (`src/foundation/cripto/include/nl/foundation/cripto/aead.h:27-34`) não tem chamador fora do motor `cripto`. O cofre (NL-3.3, `TODO.md:299`) e o cofre de campos (NL-3.4, `TODO.md:300`) estão "Pendente"; o arquivo S da ADR-0003 (regra 12, `:95`) não existe.
2. **Nenhuma migração do produto existe.** `MigracaoEmbutida` só é declarada (`src/foundation/sql/include/nl/foundation/sql/migrador.h:22-25`) e usada nos catálogos de teste (`tests/integracao/banco/tst_migrador_banco.cpp:41-57`). O único objeto que o app cria no banco é a `nl_migracao` (`src/foundation/sql/migracao_aplicacao.cpp:161-164`).
3. **Nenhum binário do produto conecta ao banco.** `nl_mid_persistencia` só é ligada pelo próprio teste (`src/mid/persistencia/CMakeLists.txt:5-8`; `tests/unit/mid/persistencia/CMakeLists.txt:5`).
4. **O marco da Fase 0 não aconteceu.** `docs/auditoria/` só tem `onda-0.md` a `onda-2.md`; não há `docs/auditoria/deploys/`.
5. **"Salvar cópia em outro lugar" e "Voltar à cópia" não estão no repositório.** A busca no plano, na spec e no `TODO.md` dá zero; os dois só existem no texto das perguntas de 04/11 (seção 0.2). Vão à INBOX (seção 10), para não virarem corte herdado (L-18).

Fatos de código que moldam o desenho:

- O executor só aceita texto literal, conferido na compilação, sem aspas, ponto e vírgula nem comentário (`src/foundation/sql/include/nl/foundation/sql/consulta.h:19-27`, `:32`, `:42`); `executarDdl` recusa dentro de transação (`executor.h:31`).
- `consultar` carrega o resultado inteiro na memória (`mysql_stmt_store_result`, `src/foundation/sql/leitura.cpp:61`).
- O leitor converte só inteiro com sinal (`leitura.cpp:22-26`) e DATE (`:28-31`); o resto volta como os bytes do buffer ligado com o tipo da coluna (`:34-47` e `:83`). INFERÊNCIA pela leitura: DATETIME volta com os bytes da estrutura `MYSQL_TIME` do conector, e BIGINT UNSIGNED acima de 2^63-1 não cabe no `qint64`. NÃO MEDIDO o que o conector escreve.
- A `nl_migracao` tem `aplicada_em DATETIME(6)` (`migracao_aplicacao.cpp:162-163`), gravada por `UTC_TIMESTAMP(6)` (`:124`, `:135`); o migrador nunca a lê (`aplicadasDe`, `:80`).
- A transação desliga o autocommit e não fixa nível de isolamento (`src/foundation/sql/transacao.cpp:20`, `:26`, `:30`, `:33`). As opções de sessão moram só em `opcoes_sessao.cpp`, com o comando inicial de `:25` (fuso `+00:00`), e o portão as conta "cada uma exatamente uma vez e só em opcoes_sessao.cpp" (`tools/portoes/sql_seguro.py:16`).
- A trava do migrador é `GET_LOCK` com o nome `nl_mig_` mais 56 dígitos do SHA-256 de `DATABASE()` (`migracao_aplicacao.cpp:26-28`, `:61-63`), numa classe interna (`:44`).
- A conexão vem da política do Mid, com o banco no `ServidorDeclarado` (`src/mid/include/nl/mid/persistencia/politica_conexao.h:21-32`, campo `banco` em `:27`; senha por `SenhaSegura::comoC()`, `src/mid/persistencia/politica_conexao.cpp:29`).
- A conta de teste tem 9 privilégios, com `CREATE TEMPORARY TABLES`, no prefixo `notalivre\_%` (`tools/conteiner/banco/sql_root.sh:34`), e a base de teste é `notalivre_teste` (`tools/conteiner/integracao_banco.sh:103`). Os testes do executor usam tabela temporária (`tests/integracao/banco/tst_executor_banco.cpp:476`).
- O job `banco` fixa 43 slots por par nas quatro linhas da matriz (`.github/workflows/ci.yml:843`, `:846`, `:849`, `:852`), com teto de 120 minutos (`:832`). O Flatpak nunca registra teste de banco (`tests/CMakeLists.txt:22-29`).
- Precedente de pasta 0700 e arquivo 0600: `src/mid/composicao/resposta_crua.cpp:198-216`. Precedente de pasta XDG com padrão: `src/mid/composicao/emissao_cli.cpp:247-248`. Precedente de relógio injetado: parâmetro `std::chrono::system_clock::time_point agora` (`src/mid/include/nl/mid/cofre/raizes_producao.h:24`).

### 0.2 Registros de 04/11/2026 fora do repositório (transcrição)

Fonte: `/var/tmp/notalivre-logs/`, sujeita à limpeza de 30 dias (NF-07). Transcrevo só o que sustenta decisão. Verbatim do líder são só as respostas entre aspas da última linha.

- `onda3-preparo-L11-ciso.md:28` (REDAÇÃO DO CISO, A1): "quem migra na abertura não pode decifrar nada. A migração estrutural trabalha só com bytes cifrados."
- `:34-36` (REDAÇÃO DO CISO, A2): local "arquivo em `$XDG_DATA_HOME/notalivre/copias/`, pasta 0700 e arquivo 0600, gerado pelo próprio app pela conexão com o banco"; cifra: "campo pessoal já cifrado por AES-256-GCM sob a chave de cada cofre, copiado byte a byte, sem chave nova. O arquivo perde a camada de cifra do próprio banco [...]: numeração, cadeia de auditoria e metadados ficam legíveis a quem lê o arquivo, sem dado pessoal e protegidos só pela permissão 0600"; verificação `copiaSemTextoClaroDaFixture`, "varrendo os bytes do arquivo pelas fixtures". O que fica desprotegido (itens 1 a 5): a cópia abre com as chaves da época; cópia levada para fora perto do `.pfx`; disco único; tomador apagado fica na cópia até ela vencer (LGPD, para o CLO); tamanho e espaço livre não medidos.
- `:103` (REDAÇÃO DO CISO, Fase 1): "contagem e hash não provam isso. Proposta: a migração trabalha sobre a cópia restaurada num banco temporário." `:111`: "o banco temporário exige permissão já concedida no script de instalação".
- `onda3-pacote-final-v4.md`, rodada 3, título `Antes` (REDAÇÃO DO CISO e do CTO; texto da pergunta): "Antes, faz uma cópia e testa se ela volta. Na cópia, os dados pessoais seguem cifrados, mas ela não tem a segunda proteção do banco: numeração e datas ficam legíveis, e ela abre com a senha da época. Fica em pasta privada do app, e um botão salva outra em outro lugar." Opções da tabela: "Confirmar, script para (Recomendado)", "Confirmar, script migra", "Cópia com senha própria" ("Cobre numeração e datas. Mais uma senha a guardar; esquecida, a cópia morre.") e "Migrar e avisar depois". Título `Descarte`: "Um botão volta à cópia." INFERÊNCIA forte de que este foi o texto respondido: o pacote v5 (posterior) dá `Antes`, `Migração` e `Descarte` por respondidas com a recomendada (`onda3-pacote-final-v5.md:270`) e cita "a resposta 'Antes' (v4)" (`:463`).
- `onda3-plano-cto.md:500-510` (REDAÇÃO DO CTO, anterior ao piso de testes de 20/11): testes previstos `copiaEIdaEVoltaIgualEmCadaByte`, `copiaAnteriorPreservadaAposMigrar`, `copiaSemTextoClaroDaFixture`, `permissaoDoArquivoE0600`, `semEspacoNaoMigra`, `caminhoNaoApareceEmLog`, `falhaNaCopiaNaoTocaOriginal`; "Fora: botão, e levar a numeração na restauração, que depende da NL-3.5. A função de restauração recebe um gancho para isso."; mutantes "arquivo 0644; copiar sem conferir o hash; apagar o original". `onda3-plano-cto-emenda.md:346`: `permissoesDoScriptBastamParaCopiar` ("com exatamente os 8 privilégios"), `copiaUsaBinarioNaCollation`, e "Cifra de arquivos no banco temporário: gancho; a sintaxe por família [...] vem com a NL-3.6".
- Respostas do líder, 04/11/2026, verbatim (NF-17, `GODS_LAWS.md:161-166`): Fase 0 "Primeiro dado real (Recomendado)"; Fase 1 "Confirmar, script para (Recomendado)"; Fase 3 "Copiar, conferir, trocar (Recomendado)"; Fase 5 "Cópia e série nova (Recomendado)"; em 25/11/2026, Fase 3 "04/11 + ensaio no CI (Recomendado)" e Fase 5 "Somar as duas (Recomendado)".

### 0.3 Pesquisa (NF-25; L-22: aprender a técnica, reescrever, citar)

- MySQL 8.4, "Prepared Statements" (https://dev.mysql.com/doc/refman/8.4/en/sql-prepared-statements.html), lida em 28/11/2026: a lista de comandos preparáveis tem "{CREATE | DROP | RENAME} DATABASE" e só "SET ROLE" entre os SET; START TRANSACTION e LOCK TABLES não estão; "Placeholders can be used for the arguments of the LIMIT clause"; o exemplo de tabela escolhida em tempo de execução monta o texto por `CONCAT`, ou seja, identificador não é vinculável.
- MariaDB, "PREPARE Statement" (https://mariadb.com/docs/server/reference/sql-statements/prepared-statements/prepare-statement), pela busca: desde a 10.6.2 todo comando é preparável, salvo PREPARE, EXECUTE e DEALLOCATE. Toda a matriz MariaDB do projeto (10.11 a 13.0) está acima. O suporte a `LIMIT ?` no MariaDB não achei em página oficial: por isso o LIMIT vai escrito no literal (DA-3.1c.4).
- MySQL 8.4, "Consistent Nonlocking Reads" (https://dev.mysql.com/doc/refman/8.4/en/innodb-consistent-read.html): "If the transaction isolation level is REPEATABLE READ (the default level), all consistent reads within the same transaction read the snapshot established by the first such read in that transaction"; "With READ COMMITTED isolation level, each consistent read within a transaction sets and reads its own fresh snapshot"; a leitura consistente não atravessa DROP TABLE nem ALTER TABLE que copia a tabela.
- MariaDB, `mariadb-dump` (https://mariadb.com/docs/server/clients-and-utilities/backup-restore-and-import-clients/mariadb-dump): `--single-transaction` "dumps the consistent state of the database at the time when BEGIN was issued, without blocking any applications", só para InnoDB, e "no other connection should use the following statements: ALTER TABLE, CREATE TABLE, DROP TABLE, RENAME TABLE, or TRUNCATE TABLE"; a saída "contains SQL statements". Técnica aprendida: uma transação de leitura e nenhum DDL concorrente, garantido aqui pela trava do migrador. Técnica recusada: restaurar executando SQL vindo de arquivo (contra a D4 do líder, plano `:698`, e a NF-34 (c)).
- Linux, open(2) (https://man7.org/linux/man-pages/man2/open.2.html): `O_TMPFILE` cria arquivo sem nome, "not subject to symlink attacks", ligado depois por `linkat` com `/proc/self/fd/N` e `AT_SYMLINK_FOLLOW`; suportado em ext2 a ext4, tmpfs, XFS (3.15), Btrfs (3.16), F2FS e ubifs. rename(2) (https://man7.org/linux/man-pages/man2/rename.2.html): `RENAME_NOREPLACE` "Return an error if newpath already exists" (EEXIST); EINVAL se o sistema de arquivos não suporta; glibc 2.28. fsync(2) (https://man7.org/linux/man-pages/man2/fsync.2.html): "Calling fsync() does not necessarily ensure that the entry in the directory containing the file has also reached disk. For that an explicit fsync() on a file descriptor for the directory is also needed."
- Qt 6, `QSaveFile` (https://doc.qt.io/qt-6/qsavefile.html): `commit()` "renames it to the final fileName"; "While being written via QSaveFile the file may have more restrictive permissions"; a página não fala de fsync. `QCborStreamReader` (https://doc.qt.io/qt-6/qcborstreamreader.html): "capable of detecting corrupt input on its own", "Most errors [...] are not recoverable", `readStringChunk` não confere UTF-8, e o exemplo de leitura "does not validate that the length is a sensible value": o teto de comprimento é do chamador.
- RFC 8949, seção 4.2.1 (https://www.rfc-editor.org/rfc/rfc8949.html): "Preferred serialization MUST be used", argumentos "as short as possible", "Indefinite-length items MUST NOT appear".
- Flatpak, "Conventions" (https://docs.flatpak.org/en/latest/conventions.html): `XDG_DATA_HOME` é `~/.var/app/<app-id>/data`, e essas variáveis "are always set by Flatpak and override any host values". Referência de comandos (https://docs.flatpak.org/en/latest/flatpak-command-reference.html): `flatpak uninstall --delete-data` "Remove app data in ~/.var/app and in the permission store".
- Não refiz o que já estava medido: DDL confirma a transação anterior (`docs/pesquisa/onda3_banco_cripto.md:77-79`); o runtime KDE 6.11 não traz cliente MySQL nem MariaDB (`:47`), então nenhum `mysqldump` está ao alcance do app; o GRANT mínimo `m2` com CREATE e DROP no prefixo cobre CREATE e DROP DATABASE no prefixo, medido por `PREPARE` de SQL e não pelo conector (`docs/pesquisa/onda3_nl-i182.md:68-74`, `:109`); um banco vazio pode sobrar de CREATE DATABASE interrompido (`:123`).

---

## 1. O que a cópia contém, como é cifrada e onde fica (pergunta 1)

### 1.1 Conteúdo (DA-3.1c.1)

- Todas as tabelas do banco do app, cada linha e cada coluna, em ordem de chave primária, pelo catálogo de cópia (1.4). Hoje só existe a `nl_migracao` (0.1, medição 2); cada fatia que criar tabela acrescenta a entrada dela, e a conferência de completude (1.4) impede esquecer.
- **Cabeçalho:** marca do formato e versão 1; instante UTC da geração, do relógio injetado (`agora`, como no precedente de 0.1); versão do app; família e versão do servidor (`VERSION()`); versão do schema N (maior versão da `nl_migracao`) e os pares (versão, checksum) de 1 a N; a lista de tabelas com as colunas (nome e tipo da cópia).
- **Por tabela:** as linhas e um rodapé com a contagem e o SHA-256 da seção. **Rodapé final:** SHA-256 de todos os bytes anteriores.
- **Fora da cópia:** o arquivo S (fora do banco, ADR-0003 regra 12, `:95`), a senha do banco (arquivo próprio, NF-30), a configuração (spec 4.16) e os logs; nenhum nome de banco, caminho, usuário ou máquina.

### 1.2 "Cifrado por campo" e "sem chave nova"

- **FATO:** nenhuma chave existe hoje (0.1, medição 1). A fatia não cifra, não decifra e não recebe chave. O que estiver cifrado no banco vai byte a byte, como bytes opacos: no futuro, os campos da NL-3.4 sob a K_campo do cofre (ADR-0003 `:41` e regra 1, `:79`), as cópias da DEK e o contêiner do A1.
- **Qual chave protege, de onde vem, como o app a obtém:** a K_campo de cada cofre, derivada da DEK daquele cofre, protege os campos pessoais. O app não precisa dela para copiar nem para restaurar, então não a obtém e não pede nada ao usuário: a cópia roda antes do login, sem cofre aberto (REDAÇÃO DO CISO, A1, seção 0.2). Quem abre os dados da cópia é um fator do cofre da época (ADR-0003 regra 4, `:82`).
- **O que fica em claro** no arquivo é o que já fica em claro no banco: ids, versões, numeração, datas, MACs. O arquivo não tem a cifra do InnoDB (C3.4, `docs/seguranca/modelo-de-ameacas.md:92`). O líder leu isso na pergunta `Antes`: "numeração e datas ficam legíveis, e ela abre com a senha da época" (seção 0.2).
- **API (DA-3.1c.2):** as funções recebem um `Executor&` já conectado ao banco real e uma fábrica de conexão para o banco temporário; nenhuma recebe senha, chave ou segredo. A fábrica usa a mesma política (`dadosDeConexao`, `politica_conexao.h:31-32`), com o mesmo `ServidorDeclarado` e só o `banco` trocado. Por isso a NF-30 não dispara: a senha do banco fica com quem monta a conexão, e a conferência do servidor vale igual nas duas conexões.

### 1.3 Formato (DA-3.1c.3)

- **CBOR** (RFC 8949) com codificação determinística (seção 4.2.1), escrito por `QCborStreamWriter` e lido por `QCborStreamReader`, do QtCore: nenhuma dependência nova (NF-05). Perfil estrito: só listas de tamanho definido, só os itens do perfil, sem número de ponto flutuante, sem item de tamanho indefinido; item fora do perfil recusa.
- **Valor por tipo da coluna** (lista de 1.4): inteiro (int64), texto (UTF-8 conferido), bytes, data (texto `AAAA-MM-DD`), instante (texto `AAAA-MM-DD HH:MM:SS.ffffff`) e nulo.
- **Tetos:** nenhum comprimento declarado maior que o resto do arquivo, conferido antes de alocar, e teto de 64 MiB por valor (DA-3.1c.3, reversível; o executor confere contra o maior valor que o schema admitir).
- **Versão:** versão de formato mais nova que a do app recusa antes de qualquer DDL, sem escrita no servidor (regra da A3, ADR-0003 regra 14, `:111`). Versões futuras do app seguem lendo a versão 1, porque a cópia fica 48 h mais 30 dias (NF-17, Fase 5). O formato congela no marco da Fase 0, como o schema v1 (P4, plano `:156`).
- **Por que não os outros:** dump em SQL (restaurar executaria SQL vindo de arquivo, contra a D4 e a NF-34 (c)); ferramenta externa (ausente no runtime, 0.3, e contra "gerado pelo próprio app"); JSON (`QJsonDocument` não é incremental e bytes iriam em base64); formato binário próprio (mais leitor nosso para proteger, sem ganho sobre um padrão com leitor mantido pelo Qt).

### 1.4 Catálogo de cópia e tipos (DA-3.1c.4 e DA-3.1c.5)

- **Por que um catálogo literal:** o executor só aceita literal (0.1), identificador não é vinculável (manual do MySQL 8.4, 0.3), a NF-34 (c) proíbe concatenar SQL e a D4 diz que o SQL só vem do binário. Então cada tabela tem uma entrada no binário: o nome literal; as colunas (nome e tipo da cópia); o SELECT da primeira página e o das páginas seguintes, ordenados pela chave primária, com o `LIMIT` escrito no literal (500 linhas no produto; o catálogo de teste usa 3, para cruzar páginas com poucas linhas); e o INSERT com um `?` por coluna. A ordem das entradas é a ordem da restauração.
- **Fim da leitura:** página vazia, nunca a contagem de linhas da página, para não depender de constante que discorde do literal. Cada tabela tem `COUNT(*)` no mesmo snapshot, igual ao lido, senão recusa (`copia.contagem_divergente`).
- **Completude, conferida na geração, antes de gravar o arquivo:** o app lê o `information_schema` do banco real e recusa, sem gerar arquivo, se houver tabela fora do catálogo, coluna fora do catálogo ou noutra ordem ou tipo, ou objeto que a cópia não sabe levar (visão, gatilho, rotina, evento, chave estrangeira, AUTO_INCREMENT, tabela sem chave primária). A cópia só leva o que sabe copiar e nunca sai pela metade. A função que decide é pura e testada sem banco (5.2).
- **O que a conta do produto enxerga:** com os 8 privilégios, ela vê visão (pelo tipo da tabela), chave estrangeira, AUTO_INCREMENT e chave primária, e a recusa vale para esses no banco real. Gatilho e rotina ela não vê. FATO, manual do MySQL 8.4: "To see information about a table's triggers, you must have the TRIGGER privilege for the table" (https://dev.mysql.com/doc/refman/8.4/en/information-schema-triggers-table.html); a rotina só aparece a quem é o DEFINER ou tem SHOW_ROUTINE, SELECT global, CREATE ROUTINE, ALTER ROUTINE ou EXECUTE (https://dev.mysql.com/doc/refman/8.4/en/information-schema-routines-table.html). INFERÊNCIA: o mesmo vale para evento e para o MariaDB. A conta também não consegue criá-los, por não ter esses privilégios. Residual declarado: gatilho, rotina ou evento criado por um administrador do servidor, fora do app, não entra na cópia nem volta na restauração. A função pura recusa os seis tipos quando os recebe; a promessa no banco real é só para o que a conta vê.
- **Banco sem schema:** banco sem `nl_migracao`, ou sem linha nela, recusa (`copia.banco_sem_schema`), sem arquivo e sem temporário, porque não há estrutura a recriar, e o `catalogoDe` recusa catálogo vazio (`src/foundation/sql/include/nl/foundation/sql/migracao.h:40`). Como o produto ainda não tem migração (0.1, medição 2), nenhum banco de produto tem o que copiar até a primeira migração, que entra a partir da NL-3.3.
- **Lista fechada de tipos:** inteiro com sinal (TINYINT a BIGINT, sem UNSIGNED); texto (VARCHAR e TEXT, utf8mb4 com `utf8mb4_bin`, o que cumpre o `copiaUsaBinarioNaCollation` da emenda: paginação e igualdade byte a byte); bytes (BINARY, VARBINARY, BLOB); DATE; DATETIME(6). Motivo: o leitor do executor não converte os outros (0.1). DATETIME(6) é lido por `CAST(coluna AS CHAR(26))` e gravado por `CAST(? AS DATETIME(6))`; a sessão tem fuso fixo `+00:00` (`opcoes_sessao.cpp:25`), e DATETIME não muda com o fuso (`docs/pesquisa/onda3_banco_cripto.md`, seção 4, item f). Dinheiro já é BIGINT em centavos (NF-19). Tipo novo no schema exige estender a cópia na fatia que o cria.
- **Catálogo do produto hoje:** só a `nl_migracao`, com restauração especial (2.1, passo h). A entrada dela mora em `src/foundation/sql`, ao lado do DDL dela; a lista do produto mora em `src/mid/persistencia`. Onde mora o schema do produto, com migrações e entradas de cópia, antes da primeira tabela de domínio, é do software-architect (INBOX, seção 10).

### 1.5 Leitura consistente (DA-3.1c.6)

- **Ordem:** a trava da base, com o mesmo nome do migrador (0.1), para cópia e migração nunca rodarem juntas; depois uma única transação de leitura (`Executor::transacao`), com todas as páginas e os `COUNT(*)`; depois a gravação do arquivo; a trava sai em todo caminho de volta.
- **FATO (manual do MySQL 8.4, 0.3):** em REPEATABLE READ, o padrão do InnoDB, toda leitura da transação vê o snapshot da primeira. O `mariadb-dump` depende do mesmo.
- **O nível de isolamento não é fixado por SQL nesta fatia:** o SET geral não está na lista de preparáveis do MySQL 8.4, e as opções de sessão só podem morar em `opcoes_sessao.cpp` (portão S2). NÃO MEDIDO: servidor configurado em READ COMMITTED. Mitigação: a trava; a exclusão de quem escreve durante a migração ("Durante isso, nenhuma nota pode ser emitida, nem por script", texto da pergunta `Migração`, REDAÇÃO DO CISO) é requisito da NL-3.1d.
- **Memória:** a paginação limita a memória à página, porque `consultar` carrega o resultado inteiro (0.1).
- **A geração nunca escreve no banco real:** só SELECT, GET_LOCK e RELEASE_LOCK.

### 1.6 Pasta privada e arquivo (DA-3.1c.7)

- **Caminho:** `$XDG_DATA_HOME/notalivre/copias/`; sem `XDG_DATA_HOME`, `$HOME/.local/share/notalivre/copias/`, no molde de `emissao_cli.cpp:247-248`. No Flatpak, o `XDG_DATA_HOME` é sempre `~/.var/app/io.github.petrinhu.notalivre/data` (0.3). É a pasta de dados do app, fora da pasta de configuração, então nunca entra na exportação de configurações (spec 4.16, `docs/superpowers/specs/2026-10-25-notalivre-design.md:395`).
- **Pasta:** criada 0700; aberta como diretório sem seguir link simbólico; tem de ser do próprio usuário; permissão mais larga é reduzida a 0700; link simbólico ou dono diferente recusa (`copia.pasta_insegura`). Mais estrita que o precedente da resposta crua (`resposta_crua.cpp:198-216`), porque a cópia leva o banco inteiro.
- **Arquivo:** nasce 0600 sem nome (`O_TMPFILE` na pasta), é escrito, recebe `fsync`, é ligado ao nome final por `linkat` (EEXIST se o nome existe: nunca sobrescreve) e a pasta recebe `fsync` (fsync(2), 0.3). Onde o sistema de arquivos não suporta `O_TMPFILE`: arquivo temporário com `O_CREAT | O_EXCL | O_NOFOLLOW`, modo 0600, e `renameat2` com `RENAME_NOREPLACE`; na falha, o app apaga só o temporário que ele criou, pelo descritor da pasta e pelo nome que ele gerou (L-53 no código do produto). O `QSaveFile` fica de fora: a documentação não fala de fsync, a permissão final é a dele e ele substitui o destino (0.3).
- **Nome:** `copia-AAAAMMDDTHHMMSSZ-<16 hexadecimais aleatórios>.nlcopia`, sem dado pessoal.
- **Esta fatia nunca apaga nem sobrescreve cópia.** Fase 5: "a cópia de antes da migração fica 30 dias na máquina do usuário, sem que o app a apague antes disso" (`GODS_LAWS.md:165`). A remoção depois do prazo é da NL-3.1d.
- **Residual:** `flatpak uninstall --delete-data` remove `~/.var/app` (0.3) e as cópias junto; o banco no servidor fica. É ato do usuário; a NL-I.195 já acompanha o mesmo efeito sobre o arquivo S e a senha do banco.

---

## 2. Restauração no banco temporário (pergunta 2)

### 2.1 Como se cria (DA-3.1c.8 e DA-3.1c.9)

- **É um banco InnoDB de verdade, no mesmo servidor,** e não tabela temporária de sessão: é ele que a NL-3.1d vai migrar e trocar (REDAÇÃO DO CISO, Fase 1, seção 0.2), e fica no mesmo sistema de arquivos (`docs/pesquisa/onda3_nl-i182.md:124`).
- **Nome reservado e literal:** `notalivre_restauro`, dentro do prefixo `notalivre\_%` do GRANT.
- **Ordem, cada passo só depois do anterior:**
  - (a) o nome do banco real tem de ser diferente do reservado, antes de qualquer DDL (`copia.nome_reservado`);
  - (b) trava de servidor própria, `GET_LOCK` com nome fixo `nl_restauro`, pela conexão do banco real, da criação ao descarte (`copia.restauracao_ocupada`); com a trava da base, a de fora vem primeiro, sempre na mesma ordem;
  - (c) conferência do arquivo (2.2, item 1), antes de qualquer comando ao servidor além da trava;
  - (d) versão de formato conhecida; versão N do schema da cópia de 1 até o tamanho do catálogo do app: N igual a 0 é `copia.banco_sem_schema` (1.4), N maior que o catálogo é `copia.schema_mais_novo`; checksums 1 a N da cópia iguais aos do catálogo (`copia.catalogo_divergente`); linha da `nl_migracao` com sucesso 0 recusa já na geração (`copia.migracao_interrompida`). A cópia leva a versão e os checksums do schema, não o DDL, então só restaura num binário cujo catálogo tenha essas N migrações;
  - (e) resto de tentativa anterior: se `notalivre_restauro` existe, vazio ou só com a tabela-marca é apagado; com qualquer outro conteúdo, recusa e nunca apaga (`copia.banco_temporario_ocupado`). Cobre o CREATE DATABASE que a recuperação de queda não protege (`onda3_nl-i182.md:123`);
  - (f) `CREATE DATABASE notalivre_restauro CHARACTER SET utf8mb4 COLLATE utf8mb4_bin` (Ddl literal) pela conexão real; segunda conexão pela fábrica, que confere `SELECT DATABASE()` igual ao reservado antes de qualquer escrita; tabela-marca `nl_restauro_marca` como primeiro objeto;
  - (g) estrutura por `migrar(temporário, as N primeiras migrações embutidas)` (`migrador.h:42-43`), nunca por SQL do arquivo;
  - (h) linhas pelos INSERT literais, uma transação por página; na `nl_migracao`, que o migrador já criou, a restauração confere versão e checksum e grava `descricao` e `aplicada_em` da cópia;
  - (i) gancho da numeração (NL-3.5, REDAÇÃO DO CTO de 04/11, seção 0.2), vazio nesta fatia.
- **Cifra do InnoDB no temporário:** gancho para a NL-3.6 (emenda do CTO, seção 0.2). A criação mora numa função só, e a comparação de estrutura não olha `CREATE_OPTIONS`, para a cifra não divergir antes de existir (DA-3.1c.16).

### 2.2 Como se confere: o que prova que a cópia é íntegra e completa (DA-3.1c.10)

A prova são três conferências, todas obrigatórias:

1. **Arquivo íntegro e o mesmo.** SHA-256 do arquivo inteiro igual ao rodapé e igual ao SHA-256 devolvido na geração, que o chamador guarda; arquivo diferente é `copia.arquivo_trocado`. É o "Hash SHA-256 do backup registrado" da Fase 1 do DEPLOY_CHECKLIST. São duas passadas: o hash vem antes do parse (precedente da NL-3.M, mutante `ORDEM_PARSE_ANTES_DO_HASH`), e a segunda passada recalcula o SHA-256 e compara de novo no fim, o que fecha a janela de troca entre as passadas.
2. **Estrutura igual.** O `information_schema` do banco real e o do temporário são iguais, menos a tabela-marca: tabelas e tipo, colunas com tipo, nulidade, padrão, charset, collation e ordem, índices com colunas e unicidade. Diferença é `copia.estrutura_divergente`; ela pega deriva do banco real feita fora das migrações.
3. **Linhas iguais, linha a linha.** Cada tabela do temporário é relida com o mesmo SELECT paginado, em ordem de chave, e comparada valor a valor, com o tipo, às linhas do arquivo, que são as lidas do original na geração. Entram as colunas em claro e as cifradas. A contagem tem de ser igual ao rodapé, e o rodapé ao `COUNT(*)` da geração. A primeira divergência para, e o erro diz a tabela pela posição no catálogo e o tipo de divergência, nunca o valor (`copia.restauracao_divergente`). Contagem e hash sozinhos não provam a restauração (REDAÇÃO DO CISO, seção 0.2); a comparação linha a linha prova.

O produto não compara ao vivo com o banco real nesta fatia: sem a exclusão de quem escreve (NL-3.1d), o real pode ter mudado depois da cópia. O teste de integração faz essa comparação ao vivo como oráculo independente (5.3).

### 2.3 Como se apaga o temporário

- `restaurar` devolve um objeto de banco de trabalho que só a restauração constrói e que só nomeia o banco reservado; `descartar` recebe só esse objeto, faz `DROP DATABASE notalivre_restauro` (Ddl literal) pela conexão real e confere no `information_schema.SCHEMATA` que ele sumiu. Falha dentro de `restaurar` descarta o que ela criou antes de devolver o erro. `provarRestauracao` é restaurar, conferir e descartar. Na NL-3.1d o objeto segue vivo, para migrar.
- DROP que falha, por exemplo com a conexão caída, deixa um resto com a tabela-marca, apagado na restauração seguinte (2.1, passo e).
- NÃO MEDIDO: DROP DATABASE interrompido no MySQL 8.0 e 8.4, cujo diretório fica fora da transação (`onda3_nl-i182.md`, seção 2), pode exigir limpeza manual. Residual, com mensagem sem caminho.

### 2.4 O que nunca acontece com o banco real nesta fatia

- Nenhum INSERT, UPDATE ou DELETE; nenhum DDL (CREATE, ALTER, DROP, RENAME, TRUNCATE) sobre ele; nenhuma migração; nenhuma troca. Só SELECT numa transação de leitura, GET_LOCK e RELEASE_LOCK, e o DDL do temporário, emitido pela conexão real nomeando só o literal reservado.
- A troca, a migração da cópia, a conferência de 100% com o original e a guarda de 48 h mais 30 dias são da NL-3.1d (plano `:698`; a NF-22 mantém a NL-3.8 depois dela).
- Prova: o slot `bancoRealIntactoDepoisDaProva` (5.3), com resumo de todas as linhas e a estrutura do banco real iguais antes e depois.

---

## 3. L-70 e NF-17 nesta fatia (pergunta 3)

### 3.1 A entrega desta fatia passa pelas 7 fases?

Não. A situação (b) da NF-17 conta do marco da Fase 0: "A L-70 passa a contar da primeira instalação com dado real, registrada em `docs/auditoria/deploys/`; antes dela, a estrutura se ajusta direto" (`GODS_LAWS.md:161`). FATO: o registro não existe (0.1, medição 4), e nenhum binário do produto conecta ao banco (medição 3). Até o marco, formato e catálogo de cópia mudam direto.

Classificação pela Fase 0, para registro:

- **Schema de banco:** não; o schema do produto não muda.
- **Apagar dado:** o único DROP é o do temporário, cujo conteúdo tem o arquivo e o original intactos, então o rollback é trivial. Mesmo assim é o caminho de código mais perigoso da fatia, e por isso tem guardas (2.1, passos a e e, e o objeto de 2.3) e mutantes (4.4).
- **Segredo, token, login, permissão e rotação de chave:** não.

A fatia é de segurança e privacidade pelo conteúdo (copia todo o dado do usuário), não por uma das sete operações.

### 3.2 O que a fatia entrega a cada fase já decidida

| Fase | Decisão do líder (verbatim) | Entrega desta fatia | Fica para |
|---|---|---|---|
| 0 | "Primeiro dado real (Recomendado)" | Versão de formato no cabeçalho; formato e catálogo congelam no marco, como o schema v1 (P4) | ADR-0009 no passo 7 |
| 1 | "Confirmar, script para (Recomendado)" | Os três itens da Fase 1 do DEPLOY_CHECKLIST (cópia de dados e schema; SHA-256 registrado; restauração testada em ambiente isolado) como funções que devolvem a evidência. O schema vai como versão e checksums das migrações, não como DDL: a estrutura volta do catálogo embutido no binário, que precisa ter as N migrações (2.1, passo d). A evidência: instante, SHA-256, tamanho, versão do schema, contagens e resultado da prova. O instante permite à NL-3.1d conferir as "últimas 2 horas" | Confirmação na abertura e script que para com o código 10: NL-3.1d, NL-3.6 e NL-3.8 |
| 2 | Troca do CLAUDE.md: dupla validação manual em staging | Não se aplica: a fatia não emite | NL-12 |
| 3 | "Copiar, conferir, trocar (Recomendado)" e "04/11 + ensaio no CI (Recomendado)" | O "copiar", já provado por restauração | Conferir 100% e trocar: NL-3.1d; ensaio a partir de cada schema publicado (NL-I.381): NL-3.1d |
| 4 | "com segundo passo opcional, opt-in, nas configuracoes"; "Guardar no Bitwarden (Recomendado)" | Nada muda; bibliotecas só do projeto (QtCore e Connector/C) | NL-3.3 |
| 5 | "Cópia e série nova (Recomendado)" e "Somar as duas (Recomendado)" | A cópia que fica 30 dias, nunca apagada nem sobrescrita por esta fatia | Remoção no prazo, as 48 h do original, "Voltar à cópia" e série nova ao restaurar sem o banco atual: NL-3.1d e INBOX |
| 6 | "Sem envio automático (Recomendado)" | A cópia nunca sai da máquina; nenhuma rede | Sem pendência |

### 3.3 O que é irreversível ou toca chave e dado, e o que isso exige no plano

1. **Formato 1 da cópia (porta P4).** Exige versão no cabeçalho, recusa de versão mais nova, congelamento no marco e a ADR-0009 (passo 7).
2. **Todo o dado do usuário num segundo lugar:** campos pessoais cifrados, metadados em claro e, no futuro, as cópias da DEK e o contêiner do A1. Exige o modelo de ameaça (4.1), a revisão do Narciso, a mutação (P2) e o teste sem texto claro da fixture (5.3).
3. **DROP DATABASE no servidor do usuário.** Exige as guardas de 2.1, cada uma em função testada sem servidor, e os mutantes M8 a M10.
4. **Privilégio para o script da NL-3.6.** Exige integração com uma conta de 8 privilégios (5.3) e a lista à NL-3.6 (INBOX). O CISO avisou: "o banco temporário exige permissão já concedida no script de instalação" (seção 0.2).

---

## 4. Segurança e privacidade (pergunta 4)

### 4.1 Modelo de ameaça curto

| Ameaça | Controle nesta fatia | Residual |
|---|---|---|
| Quem lê a pasta | Pasta 0700 e arquivo 0600: outro usuário do sistema não lê. Os campos pessoais seguem cifrados pela K_campo do cofre | O próprio usuário, ou programa dele fora do sandbox, lê, como lê a senha do banco e o arquivo S. Metadados legíveis e arquivo sem a cifra do InnoDB, residual que o líder leu na pergunta `Antes` |
| Cópia roubada (levada da máquina, backup do home) | Os dados pessoais só abrem com um fator do cofre da época | A cópia abre com a senha antiga, o código antigo e o A1 com a senha dele (C1.2 vale também para o arquivo). O aviso de cópia antiga ao trocar senha, recuperar, trocar o A1 ou revogar o headless vai à INBOX |
| Cópia adulterada | Leitor robusto: nenhum comprimento além do arquivo, perfil estrito, UTF-8 conferido, nenhum SQL do arquivo, estrutura só do binário, escrita só no temporário por parâmetro vinculado. Campo cifrado herda a tag GCM com AAD por tabela, coluna, linha e key_id (ADR-0003 regra 1): valor trocado entre linhas ou colunas falha ao abrir o cofre. Linhas com MAC da regra 12, quando a NL-3.3 existir, idem | Sem chave, o hash só pega acidente: coluna em claro alterada e linha removida passam. É a P1 |
| Troca de cópia | No fluxo da migração, a prova exige o SHA-256 devolvido na geração (`copia.arquivo_trocado`) | Fora desse fluxo ("Voltar à cópia", futuro), a escolha é do usuário; "Cópia e série nova" impede reusar número |
| Cópia antiga | Nesta fatia a cópia só vai ao temporário | Restaurar como produção é de fatia futura, com a série nova da Fase 5 |
| Apagar o banco errado | Nome literal reservado, nome real diferente antes de qualquer DDL, trava própria, resto alheio nunca apagado, descarte só do objeto que a restauração construiu | Banco criado à mão pelo usuário com o nome reservado e só com a tabela-marca seria apagado (nome documentado como reservado) |
| Escrever no banco real | Nenhuma escrita por construção; a conexão do temporário confere `DATABASE()`; teste de banco intacto | Nenhum |
| Disco cheio | Geração falha sem arquivo pela metade com nome (`copia.sem_espaco`); temporário que enche o servidor falha e é descartado | Disco único leva banco e cópia (REDAÇÃO DO CISO, A2.3) |
| Objeto do servidor criado fora do app (gatilho, rotina, evento) | A conta do produto não os cria; a conferência recusa visão, chave estrangeira e AUTO_INCREMENT, que ela vê (1.4) | O que um administrador criar fora do app não entra na cópia nem volta na restauração (1.4) |

### 4.2 Autenticação da cópia

- **O que existe:** SHA-256 contra acidente; vínculo pelo SHA-256 que o chamador guarda na geração; tag GCM dos campos cifrados; MAC da regra 12 quando a NL-3.3 existir.
- **O que fica aberto sem chave:** troca por cópia mais antiga fora do fluxo da migração, linha removida e coluna em claro alterada. Quem consegue isso tem acesso de escrita à pasta do usuário, o mesmo domínio que já permite escrever no banco (senha do banco em arquivo só do dono, NF-30; residuais C3.1 e C3.8, `docs/seguranca/modelo-de-ameacas.md:89` e `:96`).
- **Não decido sozinho nem a chave nem o aceite do risco:** é a P1 (seção 8), depois do parecer do Narciso (passo 0). Com a opção 1, nada muda nos passos 2 a 5. Com a opção 2, também não: o selo entra na NL-3.3, que cria o segredo, numa versão 2 do formato, antes do primeiro dado real. Com a opção 3, a senha própria muda o formato e a API (DA-3.1c.2 e DA-3.1c.3), e o plano volta ao CTO. Por isso os passos 2 a 5 só começam depois da resposta.

### 4.3 O que entra em log

Nada. A fatia não chama o `Log`. Devolve resultado estruturado e erro com código estável e mensagem fixa, sem caminho, nome de banco, de tabela ou de coluna e sem valor. A ADR-0005 proíbe "nome e caminho de arquivo" (`docs/adr/0005-logs-sem-pii.md:24`). O evento de log e de auditoria da cópia é da NL-3.1d, pela allowlist da ADR-0005: códigos, contagens, versão do schema, tamanho, duração e instante.

### 4.4 Mutação (NF-12)

- A fatia toca segurança e privacidade (4.1), então a mutação vale (NF-12, L-27) e vai ao líder (P2), como na NL-3.M (28/11/2026, "Teste novo e todos os mutantes (Recomendado)", `TODO.md:276`).
- **Desenho para a mutação morrer em teste de unidade (DA-3.1c.12):** toda decisão de segurança mora em função testável sem servidor, e a orquestração da prova recebe as operações (servidor, arquivo) por interface, com operações falsas no teste. Assim os mutantes rodam no job nativo ubuntu26.04, no molde de `tools/mutacao/mutante_catalogo.sh`: cópia do SHA fora do workspace, troca de uma linha com contagem exata, executável com sha diferente, morte só pela asserção exata do slot, seletor por caminho no `ci.yml`, fail-secure sem base.

| Mutante | O que o mutante faz | Slot que o mata |
|---|---|---|
| M1 PASTA_LARGA | não reduz a pasta a 0700 | `tst_pasta_copias` `pastaLargaFicaSoDoDono` |
| M2 PASTA_SEGUE_LINK | abre a pasta seguindo link simbólico | `tst_pasta_copias` `pastaQueELinkSimbolicoRecusada` |
| M3 ARQUIVO_0644 | cria o arquivo com 0644 | `tst_arquivo_copia` `arquivoNasceSoDoDono` |
| M4 SOBRESCREVE_COPIA | liga o arquivo trocando o destino | `tst_arquivo_copia` `copiaAnteriorNuncaSobrescrita` |
| M5 SEM_CONFERENCIA_DO_HASH | pula o SHA-256 da primeira passada | `tst_copia_formato` `hashErradoRecusadoAntesDoParse` |
| M6 PARSE_ANTES_DO_HASH | faz o parse antes do hash | `tst_copia_formato` `hashErradoEMalformadoRecusadoPeloHash` |
| M7 COMPRIMENTO_SEM_LIMITE | aceita comprimento além do resto do arquivo | `tst_copia_formato` `comprimentoMaiorQueOArquivoRecusado` |
| M8 SEM_GUARDA_DO_NOME | tira a guarda do nome reservado | `tst_prova_restauracao` `nomeReservadoRecusadoAntesDeQualquerDdl` |
| M9 APAGA_RESTO_ALHEIO | apaga resto sem olhar a tabela-marca | `tst_prova_restauracao` `restoAlheioRecusadoSemApagar` |
| M10 SEM_DESCARTE_NA_FALHA | o caminho de falha não descarta | `tst_prova_restauracao` `falhaEmCadaPassoDescartaOTemporario` |
| M11 SEM_CONFERENCIA_LINHA_A_LINHA | a comparação de linhas sempre aprova | `tst_prova_restauracao` `linhaDivergenteReprova` |
| M12 SEM_SHA_ESPERADO | ignora o SHA-256 da geração | `tst_prova_restauracao` `arquivoTrocadoRecusadoAntesDoServidor` |
| M13 SEM_COMPLETUDE | a conferência de completude sempre aprova | `tst_copia_catalogo` `tabelaForaDoCatalogoRecusada` |
| M14 SCHEMA_MAIS_NOVO_ACEITO | aceita N maior que o catálogo | `tst_copia_catalogo` `schemaMaisNovoQueOCatalogoRecusado` |

- O `fsync` do arquivo e da pasta não é observável em teste de unidade: um mutante que o tirasse sobreviveria. Fica na revisão do diff pelo security-engineer, declarado, sem mutante.

---

## 5. Testes e pisos (pergunta 5)

### 5.1 Regras

- Cada teste nasce vermelho antes do código (L-35, NF-33). O commit R traz os testes e esboços que devolvem `copia.nao_implementado` (precedente: "No R, migrar é esboço e cada slot falha por sql.nao_implementado", `tests/integracao/banco/tst_migrador_banco.cpp:5`). O commit G traz o código. O executor grava a saída real do vermelho e do verde (log do CI do SHA, ou contêiner local opcional) num arquivo em `/var/tmp` e cita o caminho (NF-09).
- O CI do SHA empurrado é o portão (NF-04), com base no último SHA verde da branch; como a fatia muda `CMakeLists`, roda tudo.
- Sem e2e: a fatia não tem comando de CLI (DA-3.1c.13).

### 5.2 Unidade (sem servidor: 4 distros, asan e Flatpak)

| Executável novo | Slots | Vermelho de estreia |
|---|---|---|
| `tests/unit/foundation/sql/tst_copia_formato.cpp` | `idaEVoltaDeCadaTipo` (inteiro mínimo, máximo, zero e negativo; texto com caractere de 4 bytes e maior que 256 bytes; bytes com os 256 valores; data; instante com microssegundos; nulo), `codificacaoDeterministica`, `sha256DoArquivoInteiroDevolvido`, `hashErradoRecusadoAntesDoParse`, `hashErradoEMalformadoRecusadoPeloHash`, `qualquerByteTrocadoOuCorteRecusado` (todo byte e todo corte de um arquivo pequeno), `formatoDesconhecidoRecusado`, `versaoDeFormatoMaisNovaRecusada`, `comprimentoMaiorQueOArquivoRecusado`, `itemForaDoPerfilRecusado`, `textoComUtf8InvalidoRecusado`, `rodapeDaTabelaDivergenteRecusado` | esboço devolve `copia.nao_implementado`; os 12 reprovam pela asserção |
| `tests/unit/foundation/sql/tst_copia_catalogo.cpp` | `catalogoQueCobreOBancoAceito`, `tabelaForaDoCatalogoRecusada`, `colunaForaDoCatalogoRecusada`, `tipoForaDaListaRecusado` (linhas: DECIMAL, FLOAT, DOUBLE, TIMESTAMP, DATETIME sem 6, JSON, BIT, ENUM, BIGINT UNSIGNED, texto latin1, texto com collation sem `_bin`), `objetoNaoSuportadoRecusado` (linhas: visão, gatilho, rotina, evento, chave estrangeira, AUTO_INCREMENT), `tabelaSemChavePrimariaRecusada`, `migracaoInterrompidaRecusada`, `bancoSemSchemaRecusado`, `schemaMaisNovoQueOCatalogoRecusado`, `checksumDaCopiaDiferenteDoCatalogoRecusado`, `prefixoDoCatalogoPelaVersaoDaCopia` | idem |
| `tests/unit/mid/persistencia/tst_pasta_copias.cpp` | `pastaPeloXdgDataHome`, `pastaPadraoSemXdg`, `pastaNasceSoDoDono` (com umask 000), `pastaLargaFicaSoDoDono`, `pastaQueELinkSimbolicoRecusada` | idem |
| `tests/unit/mid/persistencia/tst_arquivo_copia.cpp` | `arquivoNasceSoDoDono` (0600 com umask 000), `copiaAnteriorNuncaSobrescrita`, `falhaNaGravacaoNaoDeixaArquivo`, `semEspacoDevolveCodigoProprio` (escrita falsa que falha com ENOSPC), `nomeSemDadoPessoal`, `erroSemCaminho` | idem |
| `tests/unit/mid/persistencia/tst_prova_restauracao.cpp` (operações falsas, que registram cada chamada) | `nomeReservadoRecusadoAntesDeQualquerDdl`, `arquivoTrocadoRecusadoAntesDoServidor`, `restoVazioReaproveitado`, `restoComMarcaApagado`, `restoAlheioRecusadoSemApagar`, `sessaoForaDoTemporarioNaoEscreve`, `linhaDivergenteReprova`, `falhaEmCadaPassoDescartaOTemporario` (uma linha por passo de 2.1) | idem |

### 5.3 Integração com o banco (rótulo `banco`, os 16 pares)

- Arquivo novo `tests/integracao/banco/tst_copia_banco.cpp`, registrado como os do migrador (`tests/integracao/banco/CMakeLists.txt:46-61`), com catálogos de teste literais: duas tabelas com todos os tipos da lista e `LIMIT 3` no literal, mais a `nl_migracao`, no molde dos catálogos de `tst_migrador_banco.cpp:41-57`. O teste pode ligar o motor `cripto` para a fixture cifrada; o código da fatia nunca o liga.
- **Conta de 8 privilégios (DA-3.1c.15):** o harness cria também `notalivre_produto`, com exatamente `SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, ALTER, INDEX` no prefixo (o m2 de `onda3_nl-i182.md:68`), e todos os 14 slots da cópia conectam com ela, inclusive no arranjo de cada slot. A `notalivre_teste`, com 9 privilégios, fica para os slots de hoje. Isso prova que os 8 bastam para copiar e restaurar, o que a NL-3.6 precisa ("Permissões do script conforme a NL-3.P, provadas contra a NL-3.1", plano `:703`).
- **Banco próprio e execução em série:** cada slot da cópia usa como banco real o `notalivre_teste_copia`, criado e apagado pelo próprio slot, porque a conferência de completude recusa tabela alheia, e as tabelas dos slots do migrador ficam em `notalivre_teste`. FATO: o preset `banco` não fixa paralelismo (`CMakePresets.json`, sem `execution`), então o ctest roda em série (INFERÊNCIA: sem `CTEST_PARALLEL_LEVEL` no ambiente do harness). Mesmo assim, os 14 slots levam a propriedade `RESOURCE_LOCK` do CTest sobre `notalivre_restauro`, porque dividem o nome reservado e a trava de servidor, e em paralelo um derrubaria o outro.

| Slot | O que prova | Vermelho de estreia |
|---|---|---|
| `copiaERestauracaoIgualLinhaALinha` | gera, restaura, compara o temporário ao vivo com o original linha a linha (oráculo independente do formato), cruza páginas, confere `COUNT(*)` e descarta | esboço |
| `restauradoDecifraComAChaveDeSempre` | coluna com texto cifrado por `cifrarAes256Gcm` (`aead.h:27-28`) com chave e AAD da fixture; depois de restaurar, decifra com a mesma chave e a mesma AAD, sem chave alguma passada à cópia | esboço |
| `copiaSemTextoClaroDaFixture` | os bytes do arquivo não têm o CPF nem o nome da fixture cifrada; controle no mesmo slot: a variante com o mesmo texto numa coluna em claro faz a varredura achar, o que prova que ela enxerga | esboço |
| `bancoRealIntactoDepoisDaProva` | resumo de todas as linhas e estrutura do banco real iguais antes e depois; `notalivre_restauro` ausente no fim | esboço |
| `tabelaForaDoCatalogoNaoGeraArquivo` | tabela a mais no real: recusa, pasta sem arquivo novo | esboço |
| `estruturaDivergenteRecusada` | índice a mais no real, feito fora das migrações: `copia.estrutura_divergente`, temporário descartado | esboço |
| `restauracaoDivergenteReprova` | catálogo de teste com INSERT defeituoso (duas colunas do mesmo tipo trocadas): `copia.restauracao_divergente`, temporário descartado | esboço |
| `restoAlheioNaoEApagado` | `notalivre_restauro` criado antes com tabela alheia: recusa, banco e tabela seguem lá | esboço |
| `restoDeTentativaAnteriorEApagado` | resto com a tabela-marca: apagado, e a prova passa | esboço |
| `bancoRealComNomeReservadoRecusado` | conexão real num banco chamado `notalivre_restauro`: recusa antes de qualquer DDL, banco intacto | esboço |
| `copiaEsperaATravaDaBase` | outra sessão com a trava do migrador: recusa sem arquivo | esboço |
| `provaEsperaATravaDoTemporario` | outra sessão com `nl_restauro`: recusa sem DDL | esboço |
| `copiaDeSchemaMaisNovoRecusadaSemEscrita` | cópia com versão acima do catálogo: recusa sem criar o temporário | esboço |
| `copiaGrandeMedida` | 2.000 linhas de 4 KiB: imprime uma linha `NL-COPIA-MEDIDA` com tempo e tamanho; confere só a igualdade, sem limite de tempo (L-43: o critério por medição não é fixado aqui) | esboço |

### 5.4 Pisos e como medir

| Registro | Hoje | Efeito estimado |
|---|---|---|
| `tools/conteiner/piso_testes.txt` | 564 | +5 entradas (os 5 executáveis de unidade) |
| `flatpak/piso_suite.txt` | 432 | +5, se nenhum tiver guarda `NL_FLATPAK_BUILD`; o executor confere que `O_TMPFILE` funciona no build Flatpak e, se não, registra a exceção em `flatpak/testes_fora_do_flatpak.txt` com o motivo |
| `tests/funcoes_esperadas_por_distro.txt` | 1934 nas 4 distros, 1652 no Flatpak | 42 slots, as linhas de dados dos slots com dados e mais 2 por executável: de +55 a +80, medido pelo executor |
| `slots:` do job `banco` (`ci.yml:843`, `:846`, `:849`, `:852`) | 43 por par | +14, nas quatro linhas, no mesmo diff |

O número medido entra no mesmo diff, com a conta no comentário, como em `tests/funcoes_esperadas_por_distro.txt`. Se o primeiro run divergir, o commit seguinte corrige com o número medido. O executor do passo 6 mede o tempo do harness por par no primeiro run verde; se o teto de 120 minutos do job ficar a menos de 15 minutos, volta ao CTO antes de cortar slot.

---

## 6. Passos (pergunta 6)

Executor e revisor distintos (L-12); o main orquestra e não executa (L-10). Até 4 agentes, no máximo 2 executores ao mesmo tempo, sem dois no mesmo arquivo; um pesado por vez na trava `~/.claude/bin/pesado` (NF-07). Commits em Conventional Commits citando `NL-3.1c`; autor e SPDX da NF-10; PR da `fatia/NL-3.1c` para a `onda-3`.

| Passo | O que e onde | Executor | Revisor | Pronto quando | Espera |
|---|---|---|---|---|---|
| 0 | Parecer sobre as seções 4 e 7 deste plano e sobre a P1 e a P2, antes de irem ao líder | Narciso (CISO) | o main confere contra este plano | parecer escrito, com a recomendação para P1 e P2 | nada |
| 1 | Conta do produto no harness: função nova em `tools/conteiner/banco/sql_root.sh` (conta `notalivre_produto`, 8 privilégios), senha nova em `tools/conteiner/banco/credencial.sh` no molde das outras e conferida fora dos logs por `nl_banco_conferir_senha_fora`, as variáveis `NL_BANCO_USUARIO_PRODUTO` e `NL_BANCO_SENHA_ARQUIVO_PRODUTO` no `executor.env` de `tools/conteiner/integracao_banco.sh`, a leitura delas em `tests/integracao/banco/ambiente_banco.h`, e o autoteste do harness se ele contar contas ou arquivos | devops-sre | security-engineer | job `banco` verde nos 16 pares com os 43 slots de hoje; a terceira senha fora de todo log | nada |
| 2 | Formato e catálogo, só Foundation: `src/foundation/sql/include/nl/foundation/sql/copia_formato.h` e `copia_catalogo.h`; `copia_escrita.cpp`, `copia_leitura.cpp`, `copia_catalogo.cpp` e `copia_migracoes.cpp` (a entrada da `nl_migracao`); o `CMakeLists.txt` do motor; os dois executáveis de 5.2 e o `CMakeLists.txt` deles | backend-engineer | security-engineer (leitor de entrada não confiável) | os 23 slots vistos vermelhos no R e verdes no G; asan verde; portão de camadas verde | P1 |
| 3 | Pasta e arquivo, só Mid: `src/mid/persistencia/pasta_copias.cpp` e `arquivo_copia.cpp`, os cabeçalhos em `src/mid/include/nl/mid/persistencia/`, o `CMakeLists.txt`; os dois executáveis de 5.2 e o `CMakeLists.txt` de `tests/unit/mid/persistencia` | backend-engineer (segunda instância) | security-engineer | os 11 slots R e G; asan verde | P1; corre ao lado do 2 |
| 4 | Orquestração com operações por interface, só Mid: `banco_temporario.cpp` (nome reservado, decisão sobre o resto, objeto de banco de trabalho), `copia_seguranca.cpp` (`gerarCopia`, `restaurar`, `conferir`, `descartar`, `provarRestauracao`), `catalogo_copia_produto.cpp`; `tst_prova_restauracao.cpp` | backend-engineer | security-engineer (Narciso: guardas do DROP) e qa-engineer | os 8 slots R e G; nenhum include de `src/foundation/cripto` em `src/` | 2 e 3 |
| 5 | Adaptadores no banco: em `src/foundation/sql`, leitura paginada com `COUNT(*)`, `information_schema`, inserção e releitura (`copia_banco.h`, `copia_geracao_banco.cpp`, `copia_restauracao_banco.cpp`), e a extração da trava de `migracao_aplicacao.cpp:44-77` para `privado/.../trava_da_base.h` e `trava_da_base.cpp`, sem mudar o comportamento; em `src/mid/persistencia`, o adaptador real das operações do passo 4; `tests/integracao/banco/tst_copia_banco.cpp` e o registro em `tests/integracao/banco/CMakeLists.txt`, com `RESOURCE_LOCK` | backend-engineer | qa-engineer; tech-lead na extração da trava | os 14 slots vistos vermelhos no R e verdes no G nos 16 pares; os 12 slots do migrador verdes | 1 e 4 |
| 6 | Pisos de 5.4 no mesmo diff, com a conta; e, conforme a P2, `tools/mutacao/mutante_copia.sh` e o seletor dele no job nativo do `ci.yml` | qa-engineer (pisos); devops-sre (mutação) | tech-lead (pisos); security-engineer (mutação) | CI verde com as contagens exatas; mutação com `encontradas = aplicadas = mortas`, zero sobrevivente | 5 e P2 |
| 7 | Documentos: ADR-0009 (`docs/adr/0009-copia-do-banco-e-prova-de-restauracao.md`), com o formato 1 como implementado, o catálogo, os tipos, a pasta, o banco reservado, o que a prova prova e não prova, e a resposta da P1; linhas C3.9 (cópia em arquivo) e C3.10 (banco temporário) em `docs/seguranca/modelo-de-ameacas.md`; aceite de risco do Narciso em `AUDITORIAS.md`, se a P1 for a opção 1; `TODO.md` (NL-3.1c em "Pendente verificação" e a INBOX da seção 10); ponteiro para este arquivo no plano, em `:698` e `:724` | software-architect (ADR); technical-writer (o resto) | security-engineer (ADR, ameaças e aceite); qa-engineer (texto, sem travessão nem meia-risca) | os documentos no diff, com a P1 respondida | 5 e P1 |
| 8 | Revisões finais e merge: APROVA do qa-engineer e do Narciso no SHA final; CI do SHA lido pelo MCP do GitHub, com spot-check arquivo:linha no blob (NF-09); merge na `onda-3` só com o APROVA registrado e o SHA igual ao `<merge>^2` (NF-20) | merge por agente operacional; o main orquestra | qa-engineer | critério de pronto da fatia, abaixo | 6 e 7 |

**Ordem:** passo 0, depois P1 e P2 ao líder. O passo 1 começa junto com o 0. Os passos 2 e 3 começam juntos depois da resposta da P1, porque a opção 3 dela muda o formato e a API (4.2). Depois vêm o 4 (espera 2 e 3), o 5 (espera 1 e 4), o 6 (espera 5 e P2), o 7 (espera 5 e P1) e o 8. Cada executor confere o vermelho e o verde dos testes dele pelo log do ctest do CI. Se o piso exato parar o job antes do ctest, o executor ajusta o piso no próprio diff, com a conta, e o passo 6 confere a soma.

**Critério de pronto da fatia (L-43, escrito antes dela):**

- `gerarCopia` e `provarRestauracao` funcionam nos 16 pares com a conta de 8 privilégios.
- O arquivo é 0600, numa pasta 0700 em `$XDG_DATA_HOME/notalivre/copias/`, nunca sobrescrito, com `fsync` do arquivo e da pasta, sem o texto claro da fixture cifrada. Os campos cifrados saem iguais byte a byte e decifram com a chave de sempre.
- A prova faz as três conferências de 2.2, linha a linha. O banco real fica intacto, e o temporário some no fim e em toda falha.
- Toda recusa tem código estável e mensagem fixa, sem caminho, nome ou valor, e nenhum evento de log é novo.
- Os testes de 5.2 e 5.3 foram vistos vermelhos e depois verdes, e os testes de hoje seguem verdes em todos os jobs. Os pisos estão no mesmo diff. Os mutantes que a P2 decidir estão todos mortos.
- O Narciso e o qa-engineer deram APROVA no SHA final, e o `Status` está em "Pendente verificação".

**Ordem de serviço de cada executor (L-18):**

- **Arquivos:** os da linha dele, e só eles. Arquivo fora da linha que precise mudar para o build ou um teste passar volta ao CTO antes de ser tocado.
- **Proibido tocar:** `src/foundation/cripto` (só o teste da 5.3 pode ligá-lo); `src/mid/cofre`; `src/foundation/pkcs12`; `src/mid/composicao` e `apps/` (sem fachada nem comando); `src/foundation/sql/opcoes_sessao.cpp`, `conexao.cpp`, `modo_aberto.cpp`, `modo_fixado.cpp`, `executor.cpp`, `executor.h` e `leitura.cpp` (sem mudar a API nem o leitor do executor; o DATETIME vai à INBOX); `src/mid/composicao/registro_cli.cpp`; os pisos fora do passo 6.
- **Comportamento:** a coluna "O que e onde" e as seções 1 e 2. **Teste:** o de 5.2 e 5.3, com o vermelho e o verde gravados. **Pronto:** a coluna "Pronto quando".
- **Proibições:** sem portão novo (L-36); sem SQL concatenado nem SQL vindo de arquivo (NF-34 (c)); sem chave, senha ou segredo em parâmetro de função da cópia; sem log novo; sem valor, caminho ou nome em mensagem; sem travessão nem meia-risca; sem apagar arquivo sem caminho conferido (L-53); sem `flatpak run` fora do Builder (NF-20); sem baixar nada (NF-05); sem mutação fora do que a P2 decidir.
- **Relatório** em até 6 linhas, com o detalhe num arquivo em `/var/tmp` e o caminho no relatório. A frase-guarda vai verbatim em cada ordem.

---

## 7. Decisões técnicas do CTO (reversíveis até o marco da Fase 0)

- **DA-3.1c.1** Conteúdo: todas as tabelas do catálogo, linha a linha, em ordem de chave; campos cifrados byte a byte; nenhum cofre aberto, nenhuma chave lida ou criada; cabeçalho e rodapés de 1.1.
- **DA-3.1c.2** API sem segredo: `Executor&` do banco real e fábrica de conexão para o temporário, pela mesma política; nenhuma função recebe senha, chave ou segredo.
- **DA-3.1c.3** Formato CBOR determinístico (RFC 8949, 4.2.1), perfil estrito, versão 1; seções por tabela com rodapé de contagem e SHA-256; rodapé final com o SHA-256 do arquivo; nenhum comprimento além do resto do arquivo; 64 MiB por valor.
- **DA-3.1c.4** Catálogo literal por tabela: SELECT da primeira página e das seguintes, com `LIMIT` no literal (500 no produto), INSERT com parâmetros, ordem de restauração; fim pela página vazia; `COUNT(*)` cruzado; completude conferida na geração, recusando o que a cópia não sabe levar.
- **DA-3.1c.5** Tipos: inteiro com sinal, texto utf8mb4 com `utf8mb4_bin`, bytes, DATE e DATETIME(6) por `CAST`; o resto recusa.
- **DA-3.1c.6** Leitura numa transação, sob a trava do migrador; isolamento não fixado por SQL; a exclusão de quem escreve é da NL-3.1d.
- **DA-3.1c.7** Pasta `$XDG_DATA_HOME/notalivre/copias` 0700, conferida por descritor; arquivo 0600 por `O_TMPFILE` e `linkat`, com a alternativa `O_EXCL` e `RENAME_NOREPLACE`; `fsync` do arquivo e da pasta; nunca sobrescreve; sem `QSaveFile`; nome sem dado pessoal; a fatia nunca apaga cópia.
- **DA-3.1c.8** Banco temporário com nome reservado `notalivre_restauro`, utf8mb4 e `utf8mb4_bin`; nome real diferente antes de qualquer DDL; trava `nl_restauro`, depois da trava da base; tabela-marca; resto alheio nunca apagado; `restaurar` e `descartar` separados, com objeto que só nomeia o reservado.
- **DA-3.1c.9** Estrutura pelas N primeiras migrações embutidas, nunca por SQL do arquivo; `nl_migracao` com `descricao` e `aplicada_em` da cópia.
- **DA-3.1c.10** Prova em três conferências: arquivo (duas passadas e SHA-256 da geração), estrutura e linhas, linha a linha; sem comparação ao vivo com o real no produto.
- **DA-3.1c.11** Sem log; códigos estáveis: `copia.pasta_insegura`, `copia.gravacao`, `copia.sem_espaco`, `copia.banco_sem_schema`, `copia.migracao_interrompida`, `copia.tabela_fora_do_catalogo`, `copia.coluna_divergente`, `copia.tipo_nao_suportado`, `copia.objeto_nao_suportado`, `copia.contagem_divergente`, `copia.arquivo_corrompido`, `copia.formato_desconhecido`, `copia.formato_invalido`, `copia.arquivo_trocado`, `copia.schema_mais_novo`, `copia.catalogo_divergente`, `copia.nome_reservado`, `copia.restauracao_ocupada`, `copia.banco_temporario_ocupado`, `copia.estrutura_divergente`, `copia.restauracao_divergente`, mais `migracao.trava_ocupada` e `migracao.trava_falhou`, que já existem. O mapa para código de saída da CLI vem com o comando (NL-3.1d, NL-3.6 e NL-3.8).
- **DA-3.1c.12** Toda decisão de segurança em função testável sem servidor; orquestração por interface, com operações falsas no teste de unidade; o banco prova o adaptador.
- **DA-3.1c.13** Sem comando de CLI nem fachada em `src/mid/composicao`: a operação ainda não é função da GUI, e o portão de paridade da NL-3.8 pega a fachada quando ela nascer.
- **DA-3.1c.14** Código em `src/foundation/sql` (formato, catálogo, operações no banco, trava extraída) e em `src/mid/persistencia` (pasta, arquivo, banco temporário, catálogo do produto, orquestração); nenhum dos dois inclui `cripto`.
- **DA-3.1c.15** Conta de 8 privilégios no harness para os slots da cópia.
- **DA-3.1c.16** Ganchos: cifra do InnoDB no temporário (NL-3.6), com `CREATE_OPTIONS` fora da comparação, e numeração na restauração (NL-3.5).
- **DA-3.1c.17** Página de 500 linhas no produto; o tempo real fica medido pelo `copiaGrandeMedida`.

---

## 8. Perguntas ao líder

São duas, porque o resto é técnico e reversível (seção 7). As duas vão depois do parecer do Narciso (passo 0). Sem resposta da P1, param os passos 2 a 5 e o 7; sem resposta da P2, para só o passo 6. Os passos 0 e 1 não dependem delas.

**P1. Proteção da cópia contra alteração.** Texto para o líder:

> A cópia do banco feita antes de cada migração não tem chave própria, como o senhor decidiu em 04/11. Ela prova que não estragou por acidente e, durante a migração, que é a mesma que acabou de ser feita. Os dados pessoais seguem protegidos pela cifra de cada campo, que acusa qualquer troca. Sem chave, a cópia não prova que ninguém com acesso à sua pasta a trocou por outra mais antiga fora da migração, tirou uma linha ou mudou uma data ou um número, que ficam legíveis. Quem consegue isso já consegue mexer direto no banco.

1. **Sem chave, com o risco registrado (Recomendado).** Fica como está, e o Narciso registra o aceite do risco em `AUDITORIAS.md`. Volta a esta pergunta quando entrar o botão de salvar a cópia em outro lugar.
2. **Selo com o segredo deste computador.** Uma chave tirada do segredo que o app já guarda no computador (o arquivo S da NL-3.3) sela cada cópia. Protege a cópia levada para pendrive ou nuvem enquanto este computador existir. Com o disco perdido, a cópia levada a outro computador não confere e só volta com aviso. Entra na NL-3.3, com versão nova do formato, antes do primeiro dado real.
3. **Senha própria da cópia.** Cobre também a leitura de números e datas. Mais uma senha a guardar, pedida a cada migração; esquecida, a cópia não abre. Era uma das opções da pergunta "Antes", de 04/11, em que o senhor escolheu a recomendada.

Por que a 1: quem altera a cópia já tem o acesso que altera o banco, e a cópia não sai da pasta do app nesta fatia nem na NL-3.1d. A 2 só protege a cópia levada para fora, que ainda não existe. A 3 muda a decisão de 04/11 e põe uma senha que, esquecida, perde a cópia justo quando ela é necessária.

**P2. Mutação desta fatia (NF-12).** Texto para o líder:

> A fatia copia todos os dados e tem o único comando do app que apaga um banco: o banco temporário, depois da prova. Proponho 14 sabotagens automáticas de teste, uma em cada guarda de segurança, para provar que cada teste pega o defeito que deve pegar. Elas rodam no CI só quando a mudança tocar a cópia.

1. **Todos os 14 mutantes (Recomendado).** Lista em 4.4. Cada um morre num teste de unidade, no job nativo, pelo molde da NL-3.M. NÃO MEDIDO: o tempo; estimo de 10 a 20 minutos a mais nos pushes que tocarem a cópia.
2. **Só os 7 do banco temporário e do arquivo** (M3, M4 e M8 a M12).
3. **Sem mutação.** Só os testes novos, com o vermelho de estreia de cada um.

Por que a 1: é a postura mais segura numa fatia que guarda o banco inteiro e apaga um banco no servidor do usuário, e foi a escolha do senhor na NL-3.M. **Forma:** pergunta, como na NL-3.M; o main confere se a decisão do líder de 24/11/2026, sobre quem decide a exceção de segurança, faz desta pergunta um aviso.

---

## 9. O que não consegui medir

- Se o servidor MySQL 8.4 recusa SET em comando preparado: li só a lista do manual, e a fatia não depende disso.
- `LIMIT ?` no MariaDB: não achei página oficial, e a fatia não o usa.
- O que o leitor do executor devolve para DATETIME: INFERÊNCIA pela leitura do código.
- CREATE e DROP DATABASE pelo `mysql_stmt_prepare` do conector: a NL-I.182 mediu `PREPARE` de SQL. O primeiro run dos 16 pares mede.
- `O_TMPFILE` nos runners e no `flatpak-builder`: os testes de unidade medem.
- O tempo da cópia e da prova: o `copiaGrandeMedida` imprime, sem limite fixado.
- O servidor configurado em READ COMMITTED.
- DROP DATABASE interrompido no MySQL 8.0 e 8.4.
- O efeito de `--delete-data` além da frase da documentação.
- Se a conta de 8 privilégios enxerga evento no MySQL e gatilho, rotina e evento no MariaDB: li só as páginas do MySQL 8.4 sobre gatilho e rotina (1.4).
- Se o ambiente do harness define `CTEST_PARALLEL_LEVEL`: o `RESOURCE_LOCK` dos slots novos cobre os dois casos.
- Se o líder viu exatamente o texto do pacote v4: INFERÊNCIA forte, pelo v5 (`:270` e `:463`).

---

## 10. Fora desta fatia, e o que vai à INBOX

Itens que o líder leu nas perguntas de 04/11 ou que esta fatia cria. Cada um precisa de linha no `TODO.md` (passo 7), com o destino proposto:

- (a) **Botão "Salvar cópia em outro lugar"**, pelo portal de arquivos, com o aviso do A1 longe da cópia, e o comando de CLI equivalente (paridade). Fonte: texto da pergunta `Antes`, sem registro no repositório (0.1, medição 5). Destino: NL-3.1d ou fatia própria; revisitar a P1 nele.
- (b) **"Voltar à cópia"**: botão e passo a passo para iniciante, com os efeitos (senha da época, série nova, notas pelo ADN, headless revogado) e a CLI equivalente. Fonte: texto da pergunta `Descarte`. Destino: NL-3.1d ou fatia própria.
- (c) **NL-3.6:** o script concede exatamente os 8 privilégios, provados pela conta do passo 1; cria o banco real com utf8mb4 e `utf8mb4_bin`; nunca usa o nome reservado `notalivre_restauro`; e a cifra do InnoDB cobre também o temporário.
- (d) **Leitor do executor:** não converte DATETIME, TIMESTAMP nem UNSIGNED (`src/foundation/sql/leitura.cpp:22-47` e `:83`). O CTO decide se o leitor ganha esses tipos; até lá, o schema fica na lista fechada de DA-3.1c.5.
- (e) **Onde mora o schema do produto** (migrações e entradas de cópia), antes da primeira tabela de domínio da NL-3.3: software-architect.
- (f) **LGPD:** o tomador apagado continua na cópia até ela vencer (REDAÇÃO DO CISO, A2.4), e o cofre removido pela NF-28 também, por até 48 h mais 30 dias depois da remoção: Cláudio (CLO), com a NL-3.1d, que confere a coerência com o prazo da NF-28.
- (g) **Aviso de cópia antiga** ao trocar a senha, recuperar o cofre, trocar o A1 ou revogar o headless (REDAÇÃO DO CISO, A2.1): NL-3.3 e NL-3.9, com a NL-3.1d.
- (h) **Fuzz do leitor da cópia:** entra no TST-T3.
- (i) **Exclusão de quem escreve durante a migração**, com a emissão recusada com o código 10: NL-3.1d, com a NL-3.5 e a NL-3.7.
- (j) **Tempo da cópia e da prova com fixture realista** antes do primeiro dado real (a pergunta `Migração` avisou que "a abertura demora mais naquela versão"): NL-3.1d.
