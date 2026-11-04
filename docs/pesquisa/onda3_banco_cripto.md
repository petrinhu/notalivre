# Pesquisa de abertura da Onda 3: banco e criptografia (NL-3.P)

**ID:** NL-3.P. **Data:** 04/11/2026. **Branch:** `fatia/NL-3.P`, de `onda-3` (SHA de partida `e9866ba3721b00b608d4ca97dd86c9ec65df0a7d`).
**Escopo:** só medição e leitura. Nenhum pacote instalado, nenhum servidor com porta exposta, nenhuma decisão tomada aqui. O que segue são recomendações ao CTO.

**Rótulos (L-18, L-44):** FATO é o medido, com arquivo, comando ou fonte citada. INFERÊNCIA é conclusão não medida. NÃO MEDIDO é o que esta fatia não mediu.

**Limites desta execução:** MemAvailable lido antes de cada contêiner, entre 22024972 e 22211796 kB (piso 21000000). Disco por `btrfs filesystem usage /`: Free (estimated) min de 44.66 GiB; Device unallocated de 8.03 GiB.

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

FATO, `ostree` na imagem `notalivre/flatpak-builder-fedora44:nl-0.5`: `/usr/sbin/ostree`, versão 2026.4. Montar o repositório `/var/tmp/notalivre-flatpak-user/repo` como `:ro` dá `opendir(/repo): Permission denied`, com a identidade padrão do contêiner e com `--user 1000:1000`. SELinux em `Enforcing`; o caminho tem rótulo `user_tmp_t`. Não houve relabel (L-60).

FATO, busca por nome no deploy do SDK (99846 entradas; `os-release`: `ID=org.kde.Platform`, `VERSION_ID=6.11`): nenhum `libsodium*`, `libmariadb*`, `libmysqlclient*` nem `libqsqlmysql*`. Os 12 achados são arquivos `.m4` de `aclocal`, `cmake/Qt6/FindMySQL.cmake` e um arquivo de licença (`share/licenses/.../sodium`).

FATO, busca por nome no deploy do Platform 6.11 (57526 entradas): nenhum achado para `sodium`, `mariadb` ou `mysql`.

FATO, CI remoto: job `flatpak (build contra SDK KDE 6.11 e bundle)` do run 41683428468 (`main`, `e9866ba`) concluído com sucesso; log com "suite_flatpak: VERDE, 340 de 340 testes passaram". O log não cita libsodium nem MariaDB.

INFERÊNCIA: o SDK e o Platform 6.11 não trazem libsodium nem cliente MariaDB ou MySQL. Isso reforça o achado do plano sobre o manifesto Flatpak (seção 0). Confirmar com build é trabalho da NL-3.2a.

NÃO MEDIDO: a listagem do `ostree` (`ostree refs` e `ostree ls -R`), que o SELinux impediu. Não se mediu o conteúdo dos objetos por hash. A busca por nome vale para a árvore extraída do deploy ativo, não para o histórico do repositório.

## 4. Servidores MySQL 8.4 e MariaDB 11.8 (itens a a g do passo 6)

FATO: `mariadb:11.8` está local, 454 MB. `mysql:8.4` não está local e não foi baixada, porque a fatia parou antes da subida (ver abaixo). A imagem baixada só pelo plano, sem contêiner, não é tamanho medido.

FATO, guarda de memória dos servidores: a subida de servidor com `--memory 2g` soma ao contêiner. A regra usada nesta execução foi 21000000 kB mais 2097152 kB, ou seja 23097152 kB. MemAvailable lido em 13:16:47: 22133468 kB. Abaixo do valor: parado.

NÃO MEDIDO: itens a (versão, `innodb_flush_log_at_trx_commit`, plugin padrão), b (INSERT, CREATE TABLE e ROLLBACK), c (RENAME TABLE com a segunda falhando), d (GRANT mínimo do prefixo `notalivre\_%`), e (GET_LOCK em duas sessões), f (DATETIME e TIMESTAMP com dois fusos) e g (BLOB de 256 valores). Todos dependem de servidor no ar.

FATO, documentação do MySQL 8.4 ("SQL statements that cause an implicit commit", https://dev.mysql.com/doc/refman/8.4/en/implicit-commit.html, lida em 04/11/2026): CREATE DATABASE, CREATE TABLE, DROP DATABASE e RENAME TABLE são DDL que "implicitly end any transaction"; "Most of these statements also cause an implicit commit after executing".

FATO, documentação do MariaDB ("SQL statements Causing an Implicit Commit", https://mariadb.com/kb/en/sql-statements-that-cause-an-implicit-commit/): CREATE TABLE, RENAME TABLE e DROP TABLE: "All these statements cause an implicit commit before execution", e "even if the statement fails with an error, the transaction is committed".

INFERÊNCIA (de fonte, não de teste): o item b provavelmente mantém o INSERT após o ROLLBACK, porque CREATE TABLE faz commit antes de executar. O item c não pode ser tratado como "volta tudo" só pela transação; o comportamento real do RENAME com falha no meio é NÃO MEDIDO.

INFERÊNCIA a conferir no servidor: em MySQL 8.4 a variável `default_authentication_plugin` não existe mais. O item a deve medir `authentication_policy` e `mysql.user` (o plano previa `@@default_authentication_plugin`, com a ressalva de que podia não existir).

## 5. Autenticação do cliente (INFERÊNCIA do plano, P:689 b)

FATO, MySQL 8.4 (https://dev.mysql.com/doc/refman/8.4/en/caching-sha2-pluggable-authentication.html): "In MySQL 8.4, `caching_sha2_password` is the default authentication plugin rather than `mysql_native_password` (deprecated)". Para conectar com essa conta é preciso "either a secure connection or an unencrypted connection that supports password exchange using an RSA key pair". A exigência vale na primeira conexão depois de criar conta, trocar senha, `RENAME USER` ou `FLUSH PRIVILEGES`.

FATO, fonte secundária (trechos de código no índice de busca; não é documentação oficial, a página do Connector/C em `mariadb.com/docs` deu 404): o plugin `caching_sha2_pw` existe no Connector/C a partir de 3.0.8 (CONC-312). Com TLS ativo, a senha vai em claro; RSA só é tentado sem conexão segura. Build com GnuTLS retorna erro de RSA; o pedido de suporte GnuTLS (CONC-790) foi fechado como "Won't Do". Fontes: https://github.com/mariadb-corporation/mariadb-connector-c/blob/07f6f951e4954ed12054aaa1ab65ab45bfa9b519/plugins/auth/caching_sha2_pw.c, https://sources.debian.org/src/mariadb/1%3A11.8.3-1/libmariadb/plugins/auth/caching_sha2_pw.c, https://jira.mariadb.org/browse/CONC-790.

FATO, MariaDB (https://mariadb.com/kb/en/authentication-from-mariadb-10-4/): `root@localhost` tenta `unix_socket` primeiro.

INFERÊNCIA: a conexão do app a `mysql:8.4` por TCP em 127.0.0.1 precisa de TLS para o login funcionar sem RSA. Se o cliente for build com GnuTLS, esse caminho falha. Isso é a dúvida que o plano levantou e continua aberta.

NÃO MEDIDO: se os Connector/C do Fedora 44, do Ubuntu 26.04, do Mint e do Manjaro são builds com OpenSSL ou GnuTLS; se o login `caching_sha2_password` funciona de fato em cada um (item 7 do passo 6, não executado: o cliente não existe na imagem `build-fedora44:nl-2.0`).

FATO, cliente na imagem: `command -v mysql` e `command -v mariadb` na imagem `notalivre/build-fedora44:nl-2.0` retornam rc 1 nos dois (`NL-3.P-passo7-cliente-na-imagem.log`). Nada instalado; o item vai para a NL-3.1a, como manda o plano.

## 6. Argon2id e libsodium (ADR-0006)

FATO, cabeçalho upstream `crypto_pwhash_argon2id.h` (branch `master`, lido em 04/11/2026): `crypto_pwhash_argon2id_PASSWD_MIN 0U`, `crypto_pwhash_argon2id_PASSWD_MAX 4294967295U`, `crypto_pwhash_argon2id_OPSLIMIT_INTERACTIVE 2U`, `crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE 67108864U` (64 MiB).

FATO, documentação libsodium (https://doc.libsodium.org/password_hashing/default_phf): `crypto_pwhash_ALG_ARGON2ID13` está disponível desde a libsodium 1.0.13. `passwdlen` deve estar entre `crypto_pwhash_PASSWD_MIN` e `crypto_pwhash_PASSWD_MAX`.

NÃO MEDIDO: os mesmos valores nas versões que o pacote entrega (1.0.18 e 1.0.22), porque o cabeçalho lido é o do `master`. Também NÃO MEDIDO o tempo de `argon2id` com 64 MiB e 3 passes em contêiner: a medição é da NL-3.2b, não desta pesquisa.

## 7. Permissões, GET_LOCK, fuso horário e BLOB

NÃO MEDIDO: itens d, e, f e g do passo 6 (dependem de servidor no ar; seção 4). Nada disto foi inferido.

## O que isso muda nas ordens NL-3.1a a NL-3.1d

Recomendações ao CTO. Nenhuma decisão aqui.

- **NL-3.1a (executor):** pela documentação (seção 4), DDL faz commit implícito no MySQL 8.4 e no MariaDB 11.8. O executor não deve tratar uma sequência DDL + DML como atômica. Sugestão: transação só para DML, DDL como passo verificado. Não medido empiricamente.
- **NL-3.1b (migrador):** cada DDL registra a versão só depois de executada, com checksum. A recuperação após falha no meio se apoia no checksum, não num rollback. A forma da tabela continua sendo a P4, decisão do líder.
- **NL-3.1c (cópia):** depende de TLS ou RSA na autenticação (seção 5). A cópia não deve supor login sem TLS até o item 7 ser medido.
- **NL-3.1d (troca):** RENAME TABLE em lote não pode depender de rollback até o item c ser medido no harness (NL-3.1h). Sugestão: um RENAME por vez, com contagem conferida entre um passo e outro.
- **Pergunta `Bibliotecas`:** precisa responder três pontos medidos aqui. (1) O plugin QMYSQL do Qt 6.12 pede `libmysqlclient.so.21`, e o cliente MariaDB comum não traz esse soname (seção 2, INFERÊNCIA). (2) O login `caching_sha2_password` do MySQL 8.4 exige TLS, e o build do cliente (OpenSSL ou GnuTLS) não foi medido (seção 5). (3) O servidor de teste precisa vir da imagem `mysql:8.4` e `mariadb:11.8`, porque o pacote do Mint é MySQL 8.0 e o do Manjaro é MariaDB 13.0.2, versões diferentes das imagens.
- **Versões do servidor na matriz de integração:** a matriz precisa fixar uma versão por ambiente (imagem), e não usar o pacote da distro.
- **Memória:** a integração com servidor usa a regra da soma (23097152 kB livres, seção 4). Com a máquina na faixa de 22 GB livres medida nesta fatia, a subida de servidor fica parada até haver margem ou decisão do líder sobre o limite de 2g, que é do CTO no plano (seção 4 do plano).
- **Libsodium:** os candidatos (1.0.18 nas imagens Ubuntu e Mint, 1.0.22 em Fedora e Manjaro) estão na faixa do plano. O Flatpak não traz libsodium no SDK nem no Platform (seção 3). A NL-3.2a para no ponto do Flatpak até a pergunta `Bibliotecas` ser respondida, como prevê o plano.
