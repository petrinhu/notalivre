# Onda 3: banco, troca e séries antes do migrador (pesquisa NL-I.182)

**ID:** NL-I.182. **Data:** 25/11/2026. **Branch:** `onda-3`. **Entra antes de:** NL-3.1b (decisão do líder de 20/11/2026: "Mudar o momento das 3 (Recomendado)").
**Escopo:** leitura de documentação e de código-fonte, e medição em contêiner. Nenhuma decisão tomada aqui: o que segue são recomendações ao CTO.

**Rótulos (L-18, L-44):** FATO é o medido ou o texto oficial lido, com fonte. INFERÊNCIA é conclusão não medida, ou afirmação de terceiro. NÃO ACHEI é o que se procurou sem achar. NÃO MEDIDO é o que esta fatia não mediu.

**Onde rodou.** Numa sessão na nuvem, não na máquina do líder. O Docker foi iniciado nesta sessão. A rede da sessão recusou www.gov.br, dev.mysql.com, mariadb.com, packages.fedoraproject.org, archlinux.org e o repositório do Manjaro. Por isso, os textos oficiais vieram de três caminhos: cópias guardadas em repositórios públicos, conferidas por SHA-256 quando o projeto já tinha o hash; o repositório oficial da documentação do MariaDB; e o código-fonte oficial dos dois servidores no GitHub. O que veio só pelo índice de busca está rotulado INFERÊNCIA e precisa ser relido com esses sites liberados antes de qualquer ratificação.

**O que já estava respondido e não foi refeito:**
- a semântica do HEAD e do GET em `/dps/{id}`: `docs/pesquisa/onda3_conferencia_numeracao.md`, seção 1 (NL-3.11P);
- o commit implícito do DDL, a durabilidade (`innodb_flush_log_at_trx_commit` = 1), dois conjuntos de GRANT e um tipo de falha do RENAME em lote: `docs/pesquisa/onda3_banco_cripto.md`, seção 4 (NL-3.P).

## 1. Versões de servidor que o usuário encontra

| Distro | MySQL no repositório oficial | MariaDB no repositório oficial | "mysql" é MariaDB? | Rótulo |
|---|---|---|---|---|
| Ubuntu 26.04 | `mysql-server` 8.4.13-0ubuntu0.26.04.2 | `mariadb-server` 1:11.8.6-5ubuntu0.1 | Não | FATO, índices `Packages.gz` de archive.ubuntu.com (resolute-updates e resolute-security), lidos em 25/11/2026. Atualiza o 8.4.11 de 04/11 |
| Linux Mint 22.3 (base Ubuntu 24.04) | `mysql-server` 8.0.46-0ubuntu0.24.04.5 | `mariadb-server` 1:10.11.14-0ubuntu0.24.04.1 | Não | FATO para a base Ubuntu (noble-updates); que o Mint não sobrepõe esses pacotes é INFERÊNCIA |
| Fedora 45 | `mysql-server` 9.7.x (8.4 segue como `mysql8.4-server`) | `mariadb-server` 12.3.x (11.8 segue como `mariadb11.8-server`) | Não | INFERÊNCIA (páginas Changes/MySQL_9.7 e Changes/MariaDB_12.3 do Fedora, só pelo índice de busca). Bate com o registro de 14/11/2026 no CLAUDE.md. Número exato da build: NÃO ACHEI |
| Manjaro estável | não existe | 13.0.2 | Sim | INFERÊNCIA (página do Arch pelo índice de busca); bate com o 13.0.2 medido em 04/11 |

Contexto (FATO, notas de versão no repositório da documentação do MariaDB e tags do GitHub): MariaDB 11.8.10 e 12.3.4 saíram em 16/11/2026; a 13.0.2 é GA desde 15/09/2026. MySQL tem as tags 9.7.4 e 26.10.0.

## 2. DDL atômico e queda do servidor no meio

**MariaDB, da 10.6.1 em diante (10.11, 11.8, 12.x e 13.x).** FATO, `server/reference/sql-statements/data-definition/atomic-ddl.md` (repositório mariadb-corporation/mariadb-docs, commit d7b1575d68e3):
- vale para todos os motores, exceto S3 e particionamento;
- atômicos: CREATE TABLE, "RENAME TABLE and RENAME TABLES", ALTER TABLE (e CREATE INDEX, que "is mapped to an `ALTER TABLE` statement"), CREATE e DROP TRIGGER, VIEW e SEQUENCE, DROP TABLE de uma tabela;
- só à prova de queda: DROP TABLE com várias tabelas ("is treated as if every DROP is a separate, atomic operation") e DROP DATABASE;
- recuperação no boot pelo `ddl_recovery.log`, com até 3 tentativas por entrada;
- CREATE OR REPLACE TABLE só fica atômico na 13.2 (FATO no código: a variável `drop_before_create_or_replace` só existe na tag 13.2.1). As notas da 13.0.1 e da 13.0.2 dizem o contrário; vale o código (INFERÊNCIA);
- CREATE DATABASE: NÃO ACHEI menção a atomicidade. FATO no código (`sql/sql_db.cc`, 11.8.9): o CREATE não entra no log de recuperação. INFERÊNCIA: uma queda no meio pode deixar o diretório do banco.

**MySQL 8.0 e 8.4.** Só o InnoDB tem DDL atômico; o log de recuperação é a tabela `mysql.innodb_ddl_log` (INFERÊNCIA, página do manual lida só pelo índice de busca). FATO no texto do manual embutido no servidor (`scripts/fill_help_tables.sql`, tag mysql-8.4.13): DROP TABLE de várias tabelas com uma ausente "fails ... and no changes are made". No MariaDB é o contrário: o drop das tabelas que existem acontece (FATO, `drop/drop-table.md`). CREATE e DROP DATABASE: o diretório fica fora da transação (FATO, `sql/sql_db.cc`, 8.4.13).

**MySQL 9.1 em diante (9.7 e 26.x).** CREATE e DROP DATABASE passam a usar o log de DDL (FATO no código: `use_ddl_log_for_schema_ddl` existe a partir da tag 9.1.0). INFERÊNCIA, pelas notas da 9.1.0 vistas no índice: ficam atômicos quando todas as tabelas são de motor atômico.

## 3. Atomicidade da troca (RENAME TABLE com vários pares)

**Documentação e código.**
- FATO, MySQL (texto do manual, igual nas tags 8.0.46, 8.4.13, 9.7.4 e 26.10.0): "If any errors occur during a RENAME TABLE, the statement fails and no changes are made". Privilégios de todos os pares e travas de todos os nomes são conferidos antes de qualquer mudança (`sql/sql_parse.cc` e `sql/sql_rename.cc`). Sob LOCK TABLES, o RENAME é aceito com trava WRITE.
- FATO, MariaDB (`data-definition/rename-table.md`): "If a `RENAME TABLE` renames more than one table and one renaming fails, all renames executed by the same statement are rolled back", inclusive em queda do servidor. O desfazer é feito pelo `ddl_log_revert`, não por transação. O RENAME sob LOCK TABLES ou com transação ativa é recusado logo no início (`ER_LOCK_OR_ACTIVE_TRANSACTION`, `sql/sql_rename.cc`, 11.8.9).
- FATO, nas duas famílias: entre bancos funciona se estiverem no mesmo sistema de arquivos; gatilho não muda de banco (erro 1435); view não muda de banco.

**Medição em contêiner** (FATO; MySQL 8.4.13 e MariaDB 11.8.9-MariaDB-ubu2404, imagens `mysql:8.4` e `mariadb:11.8`; lote de 3 pares, com a falha no par do meio, salvo onde indicado):

| Caso | MySQL 8.4.13 | MariaDB 11.8.9 | Par aplicado? |
|---|---|---|---|
| C0 destino existente (controle) | 1050 | 1050 | nenhum |
| C1 origem ausente | 1146 | 1146 | nenhum |
| C2 banco de destino ausente | 1049 `Unknown database` | 1025 `Error on rename ... (errno: 168 ...)` | nenhum |
| C3a e C3b sem privilégio no destino | 1142 | 1142 | nenhum, nem o primeiro par |
| C4 LOCK TABLES READ em outra sessão, `lock_wait_timeout` 3 | espera a trava de metadados, depois 1205 | igual | nenhum |
| C5 transação aberta em outra sessão | 1205 | 1205 | nenhum |
| C6a lote entre 4 bancos, sem falha | aplicado | aplicado | os 3 |
| C6b troca do desenho da NL-3.1d, sem falha | aplicado | aplicado | os 2 |
| C6c lote entre 4 bancos, falha no último par | 1050 | 1050 | nenhum |
| C6d troca com a cópia ausente | 1146 | 1146 | nenhum |

FATO, padrão das variáveis de espera: `lock_wait_timeout` é 31536000 s no MySQL (um ano) e 86400 s no MariaDB (um dia).

**Achado para o Mint e o Ubuntu 24.04: MDEV-27027.** FATO: as notas da 10.11.16 e da 11.8.6 listam "Atomic DDL: Assertion `!param->got_error` failed upon unsuccessful multi-RENAME TABLE (MDEV-27027)", e o conserto (`Postponed_error_handler` em `sql/sql_rename.cc`) não existe na tag 10.11.14. FATO: o pacote noble `mariadb_10.11.14-0ubuntu0.24.04.1` tem 4 patches, nenhum deles sobre o 27027. INFERÊNCIA: no MariaDB que o usuário do Mint 22.3 recebe, um RENAME em lote que falha depois do primeiro par pode não desfazer os pares já feitos. NÃO MEDIDO. A imagem `mariadb:10.11` do Docker Hub tende a trazer uma versão com o conserto e por isso não reproduz o usuário do Mint (INFERÊNCIA); a medição teria de usar o pacote do noble dentro da imagem do Mint, que está na lista fechada.

## 4. Conjunto mínimo de permissões

FATO, medido nos dois servidores, tirando um privilégio por vez de m2 e rodando a lista de operações do app (logs da medição, seção RESUMO de cada rodada):
- **O mínimo é o próprio m2:** `CREATE, DROP, ALTER, SELECT, INSERT, UPDATE, DELETE, INDEX` em `` `notalivre\_%`.* ``, sem nenhum privilégio global (`SHOW GRANTS`: só `USAGE ON *.*`). Cada um dos 8 é exigido por pelo menos uma operação.
- O RENAME exige ALTER e DROP na origem, e CREATE e INSERT no destino.
- UPDATE e DELETE com WHERE exigem também SELECT (1143).
- GET_LOCK e RELEASE_LOCK passam sem privilégio. A troca da própria senha passa com m2.
- CREATE global no lugar do CREATE no prefixo libera CREATE DATABASE para qualquer nome (o controle O21, banco fora do prefixo, passou a ser aceito).

Lista de operações e origem (FATO de leitura do código; INFERÊNCIA onde o SQL ainda não existe): conexão e opções de sessão (`src/foundation/sql/conexao.cpp`, `opcoes_sessao.cpp`); trava, tabela de migrações, ALTER, CREATE e DROP INDEX, DROP TABLE (plano, NL-3.1b, parte b2 ainda não escrita); CREATE DATABASE no prefixo, cópia entre bancos, conferência de contagem, RENAME entre bancos, listagem e DROP DATABASE (plano, NL-3.1c e NL-3.1d; CLAUDE.md, Fases 3 e 5). O executor prepara todo SQL por `mysql_stmt_prepare` e recusa texto com aspas, ponto e vírgula ou comentário (`include/nl/foundation/sql/consulta.h`), então o nome da trava vai como parâmetro.

Fora da lista, se o app passar a usar (FATO):

| Uso | MySQL 8.4.13 | MariaDB 11.8.9 |
|---|---|---|
| Tabela temporária (os testes de integração usam) | exige CREATE TEMPORARY TABLES | igual |
| FOREIGN KEY | exige REFERENCES | passa sem ele |
| LOCK TABLES | exige LOCK TABLES; e não pode ser preparado (1295) | exige LOCK TABLES |
| TRIGGER com log binário ligado | 1419: exige SUPER, mesmo com TRIGGER concedido | exige TRIGGER |

## 5. E0014 entre Produção Restrita e Produção

**Resposta curta.** NÃO ACHEI texto oficial que diga se as bases são separadas ou a mesma. A INFERÊNCIA de bases separadas continua de pé, um pouco mais forte.

- FATO, Anexo I v1.01 de 09/02/2026 (cópia em `prbn021/nfse-poc` @ `426f09d`, SHA-256 `de5bc492959eadc8bfa7540e16939995924f2188f743648eaf84d3b31e9eeb7c`), aba RN, linha 148, E0014: "Conjunto de Série, Número, Código do Município Emissor e CNPJ/CPF informado nesta DPS já existe em uma NFS-e gerada a partir de uma DPS enviada anteriormente." O ambiente não entra na chave, nem no Id (linha 142, E0004).
- FATO, mesma cópia, linha 143, E0006: "Ambiente informado diverge do ambiente de recebimento para o qual o emitente enviou a DPS." Cada host recebe só a DPS do seu `tpAmb`.
- FATO (conversão de terceiro, sem hash do PDF), Guia do Painel Municipal v1.2 de 01/10/2025, seção 1.2: a Produção Restrita é "um ambiente de testes, limitado"; "os dados recebidos não têm validade jurídica"; e os painéis dos dois ambientes "devem, após a parametrização final do ambiente de produção, estarem idênticos". A parametrização de cada ambiente é mantida à parte (INFERÊNCIA a partir do texto).
- INFERÊNCIA (relatos de terceiros com chamadas reais): contadores de nNFSe diferentes por ambiente no mesmo dia (`HeidyKescilley/contask-nf`); adesão do município separada por ambiente (`GustavoOta/dfe`); documentos da Produção Restrita apagados de tempos em tempos (`victorl2/govbr-nfse-api`, sem fonte).

## 6. Várias séries do mesmo emissor, e a faixa de série

**Resposta curta.** Nos textos oficiais lidos não há proibição, limite de quantidade nem exigência de série única por emissor. A única regra oficial sobre série é a faixa por tipo de emissor, recusada com E0010. Nenhum texto oficial lido liga a série à escrituração, à apuração do Simples ou ao PGDAS-D.

- FATO, Anexo I v1.01 (cópia acima), aba Leiaute, linha 106, `serie`: "Faixas de utilização da série da DPS: 00001 a 49999 - Emissão com aplicativo pŕoprio; 50000 a 69999 - Emissor Móvel; 70000 a 79999 - Emissor Web; 80000 a 89999 - Emissão com *transcrição manual (Web)".
- FATO, mesma cópia, aba RN, linha 147, E0010 (rejeição, nível 1): "A série informada na DPS não pertence à faixa definida para o tipo de emissor utilizado para a sua emissão." Na aba RN inteira, só a E0010 e a E0014 tratam de `serie`.
- NÃO ACHEI recusa de série dentro de 00001 a 49999, nem parâmetro municipal de série no Guia do Painel Municipal v1.2. Isso responde a NL-I.176 pela leitura; a medição continua com ela.
- INFERÊNCIA (terceiros, Produção Restrita): o mesmo CNPJ emitiu com a série 1 por biblioteca e com a 70000 pelo Emissor Web, e o número da NFS-e seguiu uma sequência só (`rafael-negrao/nfse-nacional-service` @ `c228c00`); série acima de 49999 enviada por software próprio foi recusada com E0010 (`victorl2/govbr-nfse-api`).
- Ponto a conferir: não deu para reabrir o Anexo I de Produção Restrita de 20/10/2026, que o projeto tem só em `/var/tmp` na máquina do líder (SHA-256 `72b05981...`, sujeito à limpeza de 30 dias), para confirmar que a E0010 e as faixas são iguais nele.

## 7. NÃO MEDIDO e o que só uma chamada real responde

- Queda abrupta de sessão ou do servidor no meio do RENAME ou do DDL (nenhum processo foi morto).
- O MDEV-27027 no MariaDB 10.11.14 do Mint (seção 3).
- MySQL 8.0 e 9.7, MariaDB 10.11, 12.3 e 13.0 na medição: só 8.4.13 e 11.8.9 foram medidos.
- O preparo pelo conector (`mysql_stmt_prepare`); só o `PREPARE` do SQL foi medido.
- A forma real do SQL da NL-3.1b a NL-3.1d, que ainda não existe.
- **E0014 entre ambientes:** um HEAD `/dps/{id}` na Produção com o Id da DPS 9 já emitida na Produção Restrita, com o certificado do líder. Nada é emitido; é chamada ao ambiente real, decisão do líder.
- **Série dentro da faixa:** uma ou duas emissões na Produção Restrita com série diferente de 1 (por exemplo 2 e 49999).
- **Escrituração:** se o contador ou o município estranham séries diferentes do mesmo CNPJ. Só eles respondem.

## 8. Recomendações ao CTO (não são fatos nem decisões)

**NL-3.1b, migrador**
1. O DDL não volta junto com o registro da versão. Cada passo deve ser idempotente ou conferido contra o `information_schema` na abertura, porque uma queda entre o DDL e a gravação da linha de migração deixa o passo aplicado sem registro.
2. `ENGINE=InnoDB` declarado e conferido em todas as tabelas antes de migrar.
3. Fora do schema: particionamento, `CREATE OR REPLACE TABLE` e `DROP TABLE` com várias tabelas (um DROP por comando). Gatilho também, se a cópia for para outro banco (não muda de banco, e no MySQL com log binário exige SUPER).

**NL-3.1c, cópia**
4. Aceitar encontrar um banco vazio que sobrou de tentativa anterior e conferir que está vazio antes de usar: o CREATE DATABASE não é coberto na queda no MySQL 8.x nem no MariaDB.
5. Manter os bancos no mesmo sistema de arquivos (o datadir padrão atende).

**NL-3.1d, troca**
6. Um único `RENAME TABLE` com todos os pares, sem LOCK TABLES: o MariaDB recusa RENAME sob LOCK TABLES, e no MySQL o LOCK TABLES não passa pelo executor preparado. A exclusão mútua fica no GET_LOCK.
7. `lock_wait_timeout` curto na sessão da troca, e o 1205 tratado como "nada aplicado", como foi medido.
8. Não depender de um código de erro só: banco de destino ausente dá 1049 no MySQL e 1025 no MariaDB.
9. Conferir o resultado depois de toda troca, listando as tabelas dos bancos envolvidos, também quando o comando der sucesso. Isso cobre o MDEV-27027 no MariaDB 10.11.14 do Mint.
10. Nomes explícitos nas chaves estrangeiras, se elas entrarem, para evitar conflito ao renomear no MySQL.

**Assistente de banco (NL-3.6) e testes**
11. O script de GRANT dá m2 no prefixo `notalivre\_%`, sem privilégio global; REFERENCES entra só se o schema usar chave estrangeira.
12. Testes de integração com a conta do app precisam de CREATE TEMPORARY TABLES, como já faz `tools/conteiner/banco/sql_root.sh`.
13. Medir no harness NL-3.1h os casos de falha da seção 3 também em MySQL 8.0.46 e 9.7 e em MariaDB 10.11.14 (pacote do noble), 12.3 e 13.0.

**Numeração (P3, NL-3.7)**
14. Manter a série por instalação e a contagem por emissor e ambiente: nada nos textos lidos se opõe a várias séries, e a E0014 com o tratamento `DpsDuplicada` cobre colisão, se as bases não forem separadas.
15. Registrar a E0010 do Anexo I como fonte da faixa de série, ao lado do FAQ 18.1 e 18.2 citado na spec R4. Limitar a série escolhida no assistente a 00001 a 49999.
16. Copiar para `docs/pesquisa/` o trecho do Anexo I de Produção Restrita de 20/10/2026 sobre a E0010 e as faixas, antes que o `/var/tmp` do líder o apague.

**Canon**
17. As versões de MySQL citadas em `docs/pesquisa/ci_medicao.md`, `onda3_banco_cripto.md` e no NL-3.6 do plano mudaram: Ubuntu 26.04 passou de 8.4.11 para 8.4.13, e o pacote do Mint passou de 8.0.46-0ubuntu0.24.04.4 para 8.0.46-0ubuntu0.24.04.5.
18. Reler as fontes rotuladas INFERÊNCIA por falta de acesso (dev.mysql.com, packages.fedoraproject.org, archlinux.org, repositório do Manjaro, www.gov.br) antes de ratificar.

## Evidência e reprodução

- Script da medição: `tools/diagnostico/nl_i182/medir.sh` (uso: `medir.sh mysql` ou `medir.sh mariadb`), com o SQL em `tools/diagnostico/nl_i182/sql/`. Um servidor por vez, com `--memory 2g --cpus 1`, sem porta publicada no host, rede `--internal` removida no fim, senhas geradas na hora e nunca gravadas em log (conferido com `grep -F`, 0 ocorrências).
- Logs: `/var/tmp/notalivre-logs/NL-I.182-mysql.log` e `NL-I.182-mariadb.log`, na sessão na nuvem de 25/11/2026. Não são duráveis: os resultados que importam estão transcritos aqui. Conferência do main (NF-09): `RESUMO m2 completo` só com a recusa O21 nos dois logs (linhas 570 e 565); caso C2 com 1049 no MySQL (linha 7931) e 1025 no MariaDB (linha 7908); 1419 do trigger no MySQL (linha 5485).
- Imagens usadas: `mysql:8.4` (id `sha256:8c233bad33d5...`, 1088006341 bytes) e `mariadb:11.8` (id `sha256:e1d5495f4456...`), ambas da lista fechada. Para conferir que o contêiner rodava, a sessão também baixou e removeu a imagem `hello-world` (25,9 kB), fora da lista; registrado pela regra de 09/11/2026 ("Ampla, só em contêiner").

## Fontes (lidas em 25/11/2026)

- Documentação do MariaDB: https://github.com/mariadb-corporation/mariadb-docs, commit d7b1575d68e3, `server/reference/sql-statements/data-definition/` (`atomic-ddl.md`, `rename-table.md`, `drop/drop-database.md`, `drop/drop-table.md`, `create/create-table.md`, `create/create-index.md`, `create/create-database.md`), `server/reference/sql-statements/transactions/` (`wait-and-nowait.md`, `sql-statements-that-cause-an-implicit-commit.md`) e as notas de versão da 10.11.13 à 13.2.1.
- Código do MySQL: https://github.com/mysql/mysql-server, tags mysql-8.0.46, 8.4.13, 9.0.0, 9.1.0, 9.7.4 e 26.10.0 (`scripts/fill_help_tables.sql`, `sql/sql_rename.cc`, `sql/sql_db.cc`, `sql/sql_parse.cc`).
- Código do MariaDB: https://github.com/MariaDB/server, tags mariadb-10.11.14, 10.11.16, 11.8.6, 11.8.8, 11.8.9, 12.3.1, 12.3.2, 12.3.4, 13.0.2, 13.1.2 e 13.2.1 (`sql/sql_rename.cc`, `sql/sql_parse.cc`, `sql/sql_db.cc`, `sql/sys_vars.cc`).
- Ubuntu: índices `Packages.gz` de http://archive.ubuntu.com/ubuntu/dists/ (resolute, resolute-updates, resolute-security, noble-updates, noble-security) e o pacote `mariadb_10.11.14-0ubuntu0.24.04.1.debian.tar.xz`.
- Anexo I v1.01 de 09/02/2026, Manual dos Contribuintes 1.0 (SHA-256 `ac2f36e3...`, igual ao registrado pelo projeto em 27/10) e manuais de municípios: cópias em https://github.com/prbn021/nfse-poc @ `426f09d`.
- Guia do Painel Municipal v1.2 e Guia do Emissor Público Nacional Web: conversões em https://github.com/edusouza/nfse-emissor-go @ `037452d`.
- Terceiros (só INFERÊNCIA): HeidyKescilley/contask-nf @ `bd1e8bf`, rafael-negrao/nfse-nacional-service @ `c228c00`, GustavoOta/dfe @ `3228f07`, victorl2/govbr-nfse-api @ `6dad560`, iurijw/emissor-em-planilha.
- Só pelo índice de busca, páginas bloqueadas nesta sessão (INFERÊNCIA): https://dev.mysql.com/doc/refman/8.4/en/atomic-ddl.html, https://dev.mysql.com/doc/relnotes/mysql/9.1/en/news-9-1-0.html, https://fedoraproject.org/wiki/Changes/MariaDB_12.3, https://fedoraproject.org/wiki/Changes/MySQL_9.7, https://archlinux.org/packages/extra/x86_64/mariadb/.
