# ADR 0008: TLS do Connector/C no adaptador de persistência (o ENFORCE não recusa texto claro)

**Status:** ACEITO por Narciso (CISO) em 19/11/2026, decisão autônoma (L-15), a ratificar pelo líder. Proposto por Caetano (CTO), condição C5 da ratificação da v3.19 (`/var/tmp/notalivre-logs/onda3-NL-3.1h-ciso-ratifica-v319.md`, seção 5.2).
**Data:** 19/11/2026.
**Origem:** NL-3.1h (harness de integração com o banco): passo 10 na fedora45 e sabotagem sab3, logs `/var/tmp/notalivre-logs/NL-3.1h-V319-F1a-fedora45-8d7dc5c.log` e `/var/tmp/notalivre-logs/NL-3.1h-V319-F1b-fedora45-1d328ac.log`.
**Relacionadas:** NL-3.6 (assistente de banco, N7: TLS sem queda para conexão aberta e impressão fixada); INBOX NL-I.340 (E2 e E3 do CISO, primeira fatia de `src/` que chama `mysql_real_connect`) e NL-I.341 (prefixo do Connector/C).

## Contexto

- FATO, fonte do Connector/C 3.4.11 (tarball com SHA-256 `521c0712e9291fa96558df9e2ff431376a3a79329f13751896b694cae12765b4`):
  - `libmariadb/mariadb_lib.c:3772-3777`: `MYSQL_OPT_SSL_ENFORCE` só grava `use_ssl`; `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` grava `tls_allow_invalid_server_cert` com o contrário do valor dado.
  - `plugins/auth/my_auth.c:314-319`: com a verificação ligada (o padrão da 3.4) ou com impressão fixada, `use_ssl` é ligado sozinho.
  - `plugins/auth/my_auth.c:343-354`: com `use_ssl` ligado e um servidor que não anuncia TLS, a recusa "SSL is required, but the server does not support it" só sai com a verificação ligada ou com impressão fixada (`tls_fp` ou `tls_fp_list`).
  - `plugins/auth/my_auth.c:357-360`: sem nenhum dos dois, o cliente tira o pedido de TLS e a sessão segue em texto claro.
- FATO medido na fedora45 em 19/11/2026, contra o MariaDB 10.11 sem TLS:
  - sem nenhuma opção de TLS: recusa, erro 2026, "SSL is required";
  - `MYSQL_OPT_SSL_ENFORCE`=1 com `MYSQL_OPT_SSL_VERIFY_SERVER_CERT`=0: sessão aberta sem TLS;
  - impressão fixada (`MARIADB_OPT_TLS_PEER_FP`) com a verificação ligada, com a impressão certa e com a trocada: recusa, erro 2026, "SSL is required".
- INFERÊNCIA do CISO: com ENFORCE=1 e a verificação desligada, quem está no caminho pode tirar o anúncio de TLS do servidor, e o cliente desce para texto claro sem erro. O nome "enforce" engana: a opção não obriga TLS.

## Decisão

1. No adaptador de persistência (`src/mid/persistencia`, quando nascer), nenhum código usa `MYSQL_OPT_SSL_ENFORCE` como guarda de TLS. A recusa de texto claro vem da impressão fixada com a verificação ligada, que é o modo E2 do CISO. Nenhum código do adaptador desliga `MYSQL_OPT_SSL_VERIFY_SERVER_CERT`. Com a verificação desligada e sem impressão, a sessão desce para texto claro (`my_auth.c:343-360`).
2. O teste do adaptador leva, contra um servidor sem TLS (classe Nenhum da matriz de integração), a mesma prova de recusa do modo fixado do teste de integração: erro 2026, "SSL is required" na mensagem, e nenhuma sessão aberta.
3. Este registro vale para o Connector/C 3.4.11. Versão nova do conector repete a leitura das linhas citadas antes de entrar.

## Como se verifica

- Hoje (19/11/2026): `src/` e `apps/` não chamam o conector (`git grep` de `mysql_`, `MYSQL_OPT` e `MARIADB_OPT` vazio em 8d7dc5c).
- Item 1: na fatia que criar o adaptador, um portão que reprova `MYSQL_OPT_SSL_ENFORCE` em `src/` e `apps/`, e também `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` em `src/` e `apps/` fora do arquivo do modo fixado, com contagem impressa e visto vermelho antes de valer (L-36).
- Item 2: o teste nomeado na fatia do adaptador, visto vermelho antes (sabotagem: tirar a impressão fixada e desligar `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` (com ou sem `MYSQL_OPT_SSL_ENFORCE`=1, como a sab3 da NL-3.1h), e ver a sessão abrir sem TLS no servidor sem TLS).
- Item 3: a versão do conector está no manifesto Flatpak e em `tools/conteiner/modulo_mariadb.sh`; a troca de versão cita esta ADR no commit.

## Consequências

- O ENFORCE sai do vocabulário de segurança do produto: ninguém presume TLS pelo nome da opção.
- Custo zero agora: nenhum código do produto muda na NL-3.1h.
