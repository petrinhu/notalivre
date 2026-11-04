# Pesquisa de abertura da Onda 3: banco e criptografia (NL-3.P)

**ID:** NL-3.P. **Data:** 04/11/2026. **Branch:** `fatia/NL-3.P`, de `onda-3` (SHA de partida `e9866ba3721b00b608d4ca97dd86c9ec65df0a7d`).
**Escopo:** só medição e leitura. Nenhum pacote instalado no host ou em imagem, nenhum servidor com porta exposta, nenhuma decisão tomada aqui. O que segue são recomendações ao CTO.

**Rótulos (L-18, L-44):** FATO é o medido, com arquivo, comando ou fonte citada. INFERÊNCIA é conclusão não medida. NÃO MEDIDO é o que esta fatia não mediu.

**Limites desta execução:** MemAvailable lido antes de cada contêiner, entre 21533548 e 22211796 kB (piso 21000000, um servidor por vez, sem contêiner de build ao mesmo tempo). Disco por `btrfs filesystem usage /`: Free (estimated) min de 44.66 GiB; Device unallocated de 8.03 GiB.

## 1. Pacotes candidatos nas quatro imagens de build

FATO. Consulta sem instalação, um contêiner por distro (`docker run --pull=never`). Logs em `/var/tmp/notalivre-logs/NL-3.P-pacotes-<distro>.log`.

| Distro (imagem) | Cliente MariaDB | libsodium | Servidor MySQL | Servidor MariaDB | Driver Qt MySQL |
|---|---|---|---|---|---|
| Fedora 44 (`dnf -q info`) | `mariadb-connector-c-devel` 3.4.9-2.fc44 | `libsodium-devel` 1.0.22-1.fc44 | `community-mysql-server`: sem resultado no `dnf info` (rc 0, sem mensagem de erro gravada) | `mariadb-server` 11.8.8-3.fc44 | `qt6-qtbase-mysql` 6.11.2-3.fc44 |
| Ubuntu 26.04 (`apt-cache policy`) | `libmariadb-dev` 1:11.8.6-5ubuntu0.1 | `libsodium-dev` 1.0.18-2 | `mysql-server` 8.4.11-0ubuntu0.26.04.1 | `mariadb-server` 1:11.8.6-5ubuntu0.1 | não consultado |
| Mint 22.3 (imagem com base noble, 24.04) | `libmariadb-dev` 1:10.11.14-0ubuntu0.24.04.1 | `libsodium-dev` 1.0.18-1ubuntu0.24.04.1 | `mysql-server` 8.0.46-0ubuntu0.24.04.4 | `mariadb-server` 1:10.11.14-0ubuntu0.24.04.1 | não consultado |
| Manjaro (`pacman -Si`) | `mariadb-libs` 13.0.2-2 (provê `libmariadb.so=3-64` e `mariadb-connector-c`) | `libsodium` 1.0.22-1 | `mysql` não existe como pacote (`error: package 'mysql' was not found`, rc 1); o virtual `mysql` é provido por `mariadb` 13.0.2-2 | `mariadb` 13.0.2-2 | não consultado |

INFERÊNCIA: o candidato de libsodium cai na faixa 1.0.18 a 1.0.22 do plano (`ci_medicao.md:25`).
FATO sobre a imagem "Mint 22.3": a base é Ubuntu 24.04, já registrada em `docs/pesquisa/ci_medicao.md`. Por isso o servidor de MySQL do pacote é 8.0, e não 8.4. Para testar 8.4 no Mint, a fonte tem de ser a imagem `mysql:8.4`, não o pacote.
FATO: nada foi instalado em imagem nem no host.

## 2. Driver Qt SQL (QMYSQL e cliente MariaDB)

FATO, plugins do volume `notalivre-qt-6120` (Qt 6.12.0, gcc_64), em `plugins/sqldrivers/`: `libqsqlibase.so`, `libqsqlite.so`, `libqsqlmimer.so`, `libqsqlmysql.so`, `libqsqloci.so`, `libqsqlodbc.so`, `libqsqlpsql.so`. Não há plugin de MariaDB separado.

FATO, `ldd` em uma única chamada, na imagem `notalivre/build-fedora44:nl-2.0` (sem cliente de banco instalado): `libqsqlmysql.so` pede `libmysqlclient.so.21 => not found`. Os demais "not found" são de outros drivers (`libfbclient`, `libmimerapi`, `libclntsh`, `libodbc`, `libpq`) e não entram aqui.

FATO, Qt, "QMYSQL for MySQL or MariaDB 5.6 and higher" (https://doc.qt.io/qt-6/sql-driver.html, lido em 04/11/2026): no Unix é preciso o cabeçalho e a biblioteca compartilhada, `libmysqlclient` ou `libmariadb`. A página diz que "the plugin for MySQL and MariaDB are combined into one Qt plugin". QMARIADB só aparece numa linha de exemplo de log, não como driver documentado.

INFERÊNCIA: o plugin foi construído contra o soname `libmysqlclient.so.21`, e o soname do Connector/C do MariaDB é outro (`libmariadb.so.3`, FATO pelo `Provides` do Manjaro). Então um cliente MariaDB comum não satisfaz esse plugin sem link de compatibilidade ou rebuild.

NÃO MEDIDO: se algum pacote de cliente MariaDB das quatro distros fornece `libmysqlclient.so.21`. Isso exigiria instalar, o que não foi feito.

## 3. SDK e Platform KDE 6.11 (libsodium, MariaDB, qsqlmysql)

FATO, premissa do plano corrigida (seção 1.2, passo 5): `/var/tmp/notalivre-flatpak-user/repo` não guarda o SDK. Listagem por `/usr/bin/find`: `objects/` vazio, `refs/heads` e `refs/remotes` sem entradas, só `config` (68 bytes). O plano dizia "conteúdo por hash"; a medição mostra o contrário.

FATO, onde o SDK está: ref `runtime/org.kde.Sdk/x86_64/6.11` em `/home/petrus/.local/share/flatpak/repo`, com deploy em `/home/petrus/.local/share/flatpak/runtime/org.kde.Sdk/x86_64/6.11/df314f9c067cba031e153a2f3b0be53bd13c1eca6baa78b68982a1d35b873a86/files`.

FATO, `ostree` na imagem `notalivre/flatpak-builder-fedora44:nl-0.5`: `/usr/sbin/ostree`, versão 2026.4. Montar o repositório `/var/tmp/notalivre-flatpak-user/repo` como `:ro` dá `opendir(/repo): Permission denied`, com a identidade padrão do contêiner e com `--user 1000:1000`. SELinux em `Enforcing`; o caminho tem rótulo `user_tmp_t`. Não houve relabel (L-60), e a negação não foi contornada.

FATO, busca por nome no deploy do SDK (99846 entradas; `os-release`: `ID=org.kde.Platform`, `VERSION_ID=6.11`): nenhum `libsodium*`, `libmariadb*`, `libmysqlclient*` nem `libqsqlmysql*`. Os 12 achados são arquivos `.m4` de `aclocal`, `cmake/Qt6/FindMySQL.cmake` e um arquivo de licença (`share/licenses/.../sodium`).

FATO, busca por nome no deploy do Platform 6.11 (57526 entradas): nenhum achado para `sodium`, `mariadb` ou `mysql`.

FATO, CI remoto: job `flatpak (build contra SDK KDE 6.11 e bundle)` do run 41683428468 (`main`, `e9866ba`) concluído com sucesso; log com "suite_flatpak: VERDE, 340 de 340 testes passaram". O log não cita libsodium nem MariaDB.

INFERÊNCIA: o SDK e o Platform 6.11 não trazem libsodium nem cliente MariaDB ou MySQL. Isso reforça o achado do plano sobre o manifesto Flatpak (seção 0). Confirmar com build é trabalho da NL-3.2a.

NÃO MEDIDO: a listagem do `ostree` (`ostree refs` e `ostree ls -R`), que o SELinux impediu. Não se mediu o conteúdo dos objetos por hash. A busca por nome vale para a árvore extraída do deploy ativo, não para o histórico do repositório.

## 4. Servidores MySQL 8.4 e MariaDB 11.8 (passo 6, itens a a g)

FATO, imagens: `mysql:8.4` baixada nesta fatia, 1098499869 bytes (ID `sha256:a81d043930e51745522bd5c430961cf8286f0a23f0a9a2a6e0108f7ea71b0902`); `mariadb:11.8` já estava local, 450201140 bytes. Logs: `NL-3.P-servidor-mysql.log` e `NL-3.P-servidor-mariadb.log`.

FATO, memória e contêineres: um servidor por vez, com `--memory 2g --cpus 1`, sem porta no host, em rede interna `nl3p-net` criada e removida nesta fatia. Piso de 21000000 kB em cada `docker run`, conforme a decisão do coordenador desta execução (a soma de 23000000 kB vale só com contêiner de build ao mesmo tempo, o que não ocorreu). Menor leitura registrada antes de contêiner: 21533548 kB. Contêineres parados com `docker stop` e removidos com `docker rm`, sem `kill`. Senhas geradas na hora, passadas por variável de ambiente, e a conferência por `grep -F` dos dois logs deu 0 ocorrências.

| Item | MySQL 8.4.13 | MariaDB 11.8.9 |
|---|---|---|
| a. Versão, `innodb_flush_log_at_trx_commit`, plugin padrão | `innodb_flush_log_at_trx_commit` = 1. `@@default_authentication_plugin` não existe (ERROR 1193). `@@authentication_policy` = `*,,`. `root@%` e `root@localhost` com `caching_sha2_password` | `innodb_flush_log_at_trx_commit` = 1. `@@default_authentication_plugin` e `@@authentication_policy` não existem (ERROR 1193). `root@localhost` e `root@%` com `mysql_native_password` |
| b. INSERT, CREATE TABLE, ROLLBACK | Após o ROLLBACK, a tabela tinha 1 linha: o INSERT ficou. Controle (INSERT e ROLLBACK sem DDL): contagem não mudou. O rollback normal funciona, então o commit veio do DDL | Igual: 1 linha após o ROLLBACK com DDL, e o controle sem DDL não mudou |
| c. RENAME em lote, segundo par com destino existente | ERROR 1050 "Table 't2' already exists". Depois: `a_rn.t1` e `a_rn.t2` seguem em `a_rn`, e `b_rn.t1` não existe. Nenhum par foi aplicado | Igual: ERROR 1050 e nenhum par aplicado |
| d. GRANT no prefixo `notalivre\_%` | Usuário m1 com CREATE e DROP: cria banco e tabela, apaga; INSERT negado (ERROR 1142); RENAME negado por falta de ALTER (ERROR 1142). Usuário m2 com CREATE, DROP, ALTER, SELECT, INSERT, UPDATE, DELETE, INDEX: as oito operações de teste passam. O ERROR 1051 de `DROP TABLE notalivre_p2.t` é consequência do RENAME negado | Igual. m1 negado em INSERT e RENAME; m2 passa em tudo |
| e. GET_LOCK em duas sessões | Sessão B sem espera recebe 0. Com espera de 15 s, recebe 1 após 6 s, quando a sessão A (que segurava o lock e dormia 8 s) terminou sozinha. Depois, `IS_FREE_LOCK` = 1 | Igual |
| f. DATETIME e TIMESTAMP com dois `time_zone` | Gravados em sessão UTC como 12:00:00. Lidos em -03:00: DATETIME 12:00:00 (não mudou), TIMESTAMP 09:00:00 (convertido) | Igual |
| g. BLOB com 256 valores de byte | Ida e volta em hexadecimal: IGUAL, 256 bytes | Igual: IGUAL, 256 bytes |

Notas:
- O item c foi medido para um único tipo de falha (destino existente). Outros tipos de falha no meio do RENAME em lote: NÃO MEDIDO.
- O item d não mediu o conjunto mínimo de GRANT. Foram testados apenas dois conjuntos (m1 e m2). Que privilégio sobra além de CREATE, DROP, ALTER e INSERT: NÃO MEDIDO.
- O item e mediu a liberação com a sessão terminando por fim normal. A liberação por queda abrupta (conexão cortada, processo morto) não foi medida, porque a ordem proíbe matar processo.
- O item f não mediu fusos nomeados (por exemplo `America/Recife`), só deslocamento numérico.

FATO, documentação do MySQL 8.4 ("SQL statements that cause an implicit commit", https://dev.mysql.com/doc/refman/8.4/en/implicit-commit.html, lida em 04/11/2026): CREATE DATABASE, CREATE TABLE, DROP DATABASE e RENAME TABLE são DDL que "implicitly end any transaction"; "Most of these statements also cause an implicit commit after executing". O item b confirmou o efeito no servidor.

FATO, documentação do MariaDB ("SQL statements Causing an Implicit Commit", https://mariadb.com/kb/en/sql-statements-that-cause-an-implicit-commit/): "All these statements cause an implicit commit before execution", e "even if the statement fails with an error, the transaction is committed". O item b confirmou no MariaDB 11.8.9.

## 5. Autenticação do cliente (passo 7, INFERÊNCIA do plano P:689 b)

FATO, cliente: a imagem `mariadb:11.8` traz `mariadb` 11.8.9 (`client 15.2 for debian-linux-gnu`). A imagem de build `notalivre/build-fedora44:nl-2.0` não tem cliente de banco (`command -v mysql` e `command -v mariadb` com rc 1 nos dois; `NL-3.P-passo7-cliente-na-imagem.log`). Por isso o cliente usado foi o da imagem MariaDB, em contêiner próprio na mesma rede interna, contra o servidor `nl3p-mysql` (MySQL 8.4.13).

FATO, resultados de login contra o MySQL 8.4.13 (log `NL-3.P-servidor-mysql.log`):

| Conta e modo | Resultado |
|---|---|
| `root`, sem TLS (`--skip-ssl`) | Login OK. `CURRENT_USER()` = `root@%`. `@@ssl_cipher` = NULL, portanto sem TLS |
| `root`, com TLS (`--ssl`) | Recusa, rc 1: `TLS/SSL error: self-signed certificate in certificate chain` |
| `notalivre_m1`, sem TLS (`--skip-ssl`) | Login OK. `CURRENT_USER()` = `notalivre_m1@%`. Sem TLS |
| `notalivre_m1`, com TLS (`--ssl`) | Recusa, rc 1, mesma mensagem de certificado autoassinado |

FATO: a conta do servidor usa `caching_sha2_password` (seção 4, item a). Com TLS, o handshake chegou até a verificação do certificado: a recusa é de confiança no certificado, e não de falta de suporte a TLS no cliente.

FATO, documentação do MySQL 8.4 (https://dev.mysql.com/doc/refman/8.4/en/caching-sha2-pluggable-authentication.html): "In MySQL 8.4, `caching_sha2_password` is the default authentication plugin rather than `mysql_native_password` (deprecated)". Para conectar com essa conta é preciso "either a secure connection or an unencrypted connection that supports password exchange using an RSA key pair". A exigência vale na primeira conexão depois de criar conta, trocar senha, `RENAME USER` ou `FLUSH PRIVILEGES`.

FATO, fonte secundária (trechos de código no índice de busca; a página do Connector/C em `mariadb.com/docs` deu 404): o plugin `caching_sha2_pw` existe no Connector/C a partir de 3.0.8 (CONC-312). Com TLS ativo, a senha vai em claro; RSA só é tentado sem conexão segura. Build com GnuTLS retorna erro de RSA; o pedido de suporte GnuTLS (CONC-790) foi fechado como "Won't Do". Fontes: https://github.com/mariadb-corporation/mariadb-connector-c/blob/07f6f951e4954ed12054aaa1ab65ab45bfa9b519/plugins/auth/caching_sha2_pw.c, https://sources.debian.org/src/mariadb/1%3A11.8.3-1/libmariadb/plugins/auth/caching_sha2_pw.c, https://jira.mariadb.org/browse/CONC-790.

FATO, MariaDB (https://mariadb.com/kb/en/authentication-from-mariadb-10-4/): `root@localhost` tenta `unix_socket` primeiro. Na imagem `mariadb:11.8` testada, a conta root foi criada com `mysql_native_password` pela variável de ambiente da imagem.

INFERÊNCIA, a hipótese do plano dizia que o cliente precisaria de TLS. A medição refuta a forma como estava escrita: o login sem TLS funcionou com `caching_sha2_password`. Como a primeira conexão de conta pede conexão segura ou RSA, o login sem TLS é compatível com a troca por RSA; a mecânica usada (RSA ou cache) é NÃO MEDIDA.

INFERÊNCIA, o caminho com TLS está quebrado por confiança, e não por protocolo: o certificado gerado automaticamente pelo servidor é autoassinado, e o cliente o recusa por padrão. Para o app, isso leva a uma decisão de confiança (CA própria, certificado fixado ou verificação desligada), que é do CTO. Não medido: se `--ssl-verify-server-cert=0` ou um certificado com CA conhecida funciona nesta combinação.

NÃO MEDIDO: se o Connector/C do Fedora 44, do Ubuntu 26.04, do Mint e do Manjaro são builds com OpenSSL ou GnuTLS, e se o login funciona em cada um. Só o cliente 11.8.9 da imagem MariaDB foi testado.

## 6. Argon2id e libsodium (ADR-0006)

FATO, cabeçalho upstream `crypto_pwhash_argon2id.h` (branch `master`, lido em 04/11/2026): `crypto_pwhash_argon2id_PASSWD_MIN 0U`, `crypto_pwhash_argon2id_PASSWD_MAX 4294967295U`, `crypto_pwhash_argon2id_OPSLIMIT_INTERACTIVE 2U`, `crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE 67108864U` (64 MiB).

FATO, documentação libsodium (https://doc.libsodium.org/password_hashing/default_phf): `crypto_pwhash_ALG_ARGON2ID13` está disponível desde a libsodium 1.0.13. `passwdlen` deve estar entre `crypto_pwhash_PASSWD_MIN` e `crypto_pwhash_PASSWD_MAX`.

NÃO MEDIDO: os mesmos valores nas versões que o pacote entrega (1.0.18 e 1.0.22), porque o cabeçalho lido é o do `master`. Também NÃO MEDIDO o tempo de `argon2id` com 64 MiB e 3 passes em contêiner: a medição é da NL-3.2b, não desta pesquisa.

## 7. O que ficou NÃO MEDIDO nesta fatia

- Listagem do repositório Flatpak pelo `ostree` (SELinux, seção 3).
- Conjunto mínimo de GRANT para o prefixo `notalivre\_%` (seção 4, item d).
- Liberação do GET_LOCK por queda abrupta de sessão (seção 4, item e).
- Caminho TLS com verificação de certificado (seção 5).
- Mecânica exata do login sem TLS (RSA ou cache) (seção 5).
- Builds OpenSSL ou GnuTLS dos clientes de cada distro (seção 5).
- Libmysqlclient no pacote MariaDB (seção 2).
- Argon2id nas versões de pacote (seção 6).

## O que isso muda nas ordens NL-3.1a a NL-3.1d

Recomendações ao CTO. Nenhuma decisão aqui. Os efeitos agora são medidos nos dois servidores, não só lidos na documentação.

- **NL-3.1a (executor):** medido (seção 4, item b): DDL confirma a transação anterior nos dois servidores, e um ROLLBACK não desfaz o INSERT feito antes do CREATE TABLE. O executor não deve tratar DDL e DML como atômicos juntos. Sugestão: transação só para DML, e DDL como passo com verificação. Não medido se o CREATE TABLE do teste foi persistido, porque a contagem verificou só o INSERT.
- **NL-3.1b (migrador):** o registro da versão deve vir depois de cada DDL, com checksum; a recuperação após falha no meio conta com os passos já aplicados. A forma da tabela continua sendo a P4, decisão do líder.
- **NL-3.1c (cópia):** a cópia depende da conexão. Sem TLS, o login funcionou nesta combinação; com TLS, a recusa é por confiança no certificado. Decisão de confiança é do CTO antes de a cópia contar com TLS (seção 5).
- **NL-3.1d (troca):** medido (seção 4, item c) para destino existente: o RENAME em lote não aplicou nenhum par. Isso não vale para outros tipos de falha, que não foram medidos. Sugestão: manter a conferência de contagem entre passos e medir mais falhas no harness NL-3.1h antes de depender do comportamento.
- **Privilégios do app (item d):** o conjunto de m2 (CREATE, DROP, ALTER, SELECT, INSERT, UPDATE, DELETE, INDEX, no prefixo `notalivre\_%`) passou em todas as operações testadas, sem FILE, SUPER nem GRANT OPTION. ALTER é necessário para RENAME (medido). Conjunto mínimo exato: NÃO MEDIDO.
- **GET_LOCK (item e):** se a migração depender de GET_LOCK para exclusão mútua, o lock vale enquanto a sessão estiver viva e some quando ela termina. Liberação por queda abrupta: NÃO MEDIDO.
- **Fuso horário (item f):** DATETIME não converte; TIMESTAMP converte pelo fuso da sessão. Campos de data de emissão e competência precisam de um tipo escolhido com esse efeito em mente. Decisão do CTO.
- **BLOB (item g):** 256 valores de byte voltam iguais nos dois servidores.
- **Pergunta `Bibliotecas`:** precisa responder quatro pontos medidos aqui. (1) O plugin QMYSQL do Qt 6.12 pede `libmysqlclient.so.21`, e o cliente MariaDB comum não traz esse soname (seção 2, INFERÊNCIA). (2) A conexão sem TLS funcionou com o cliente da imagem MariaDB 11.8.9, mas com TLS a recusa é por certificado autoassinado, e a política de confiança é do CTO (seção 5). (3) O servidor de teste deve vir da imagem `mysql:8.4` (8.4.13) e `mariadb:11.8` (11.8.9), porque o pacote do Mint é MySQL 8.0 e o do Manjaro é MariaDB 13.0.2. (4) O Flatpak não traz libsodium nem cliente de banco (seção 3).
- **Versões do servidor na matriz de integração:** a matriz fixa uma versão por ambiente (imagem), e não usa o pacote da distro.
- **Memória:** a integração com servidor segue um servidor por vez, com piso de 21000000 kB. Dentro desse piso, as leituras desta fatia ficaram entre 21533548 e 22211796 kB, com margem de cerca de 0,5 GB. Por isso, a integração com servidor fica sem folga para um segundo contêiner.
- **Libsodium:** os candidatos (1.0.18 nas imagens Ubuntu e Mint, 1.0.22 em Fedora e Manjaro) estão na faixa do plano. A NL-3.2a para no ponto do Flatpak até a pergunta `Bibliotecas` ser respondida, como prevê o plano.
