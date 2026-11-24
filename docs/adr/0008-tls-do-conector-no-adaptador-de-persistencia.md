# ADR 0008: TLS do Connector/C no adaptador de persistência (o ENFORCE não recusa texto claro)

**Status:** ACEITO por Narciso (CISO) em 19/11/2026, decisão autônoma (L-15), a ratificar pelo líder. Proposto por Caetano (CTO), condição C5 da ratificação da v3.19 (`/var/tmp/notalivre-logs/onda3-NL-3.1h-ciso-ratifica-v319.md`, seção 5.2).
**Correção:** 20/11/2026, pela leitura prévia do CISO para a NL-3.1a, seção 5 (autocorreção do ajuste B), e pelas decisões do líder de 20/11/2026, 01:43.
**Data:** 19/11/2026.
**Origem:** NL-3.1h (harness de integração com o banco): passo 10 nas 4 distros, sabotagens sab1, sab2 e sab3 e mutante M6, logs `/var/tmp/notalivre-logs/NL-3.1h-V319-F1a-fedora45-8d7dc5c.log`, `/var/tmp/notalivre-logs/NL-3.1h-V319-F1b-fedora45-1d328ac.log` e `/var/tmp/notalivre-logs/NL-3.1h-V320-sab1-fedora45-9a682be.log`. NL-3.1a: bloco Q do CISO (`/var/tmp/notalivre-logs/onda3-NL-3.1a-ciso-bloco-Q.md`).
**Relacionadas:** NL-3.6 (assistente de banco, N7: TLS sem queda para conexão aberta e impressão fixada); NL-I.339 (MariaDB com `require_secure_transport=ON` no modo fixado); INBOX NL-I.340 (E2 e E3 do CISO, primeira fatia de `src/` que chama `mysql_real_connect`), NL-I.341 (prefixo do Connector/C) e NL-I.349 (portão).

## Contexto

- FATO, fonte do Connector/C 3.4.11 (tarball com SHA-256 `521c0712e9291fa96558df9e2ff431376a3a79329f13751896b694cae12765b4`):
  - `libmariadb/mariadb_lib.c:3772-3777`: `MYSQL_OPT_SSL_ENFORCE` só grava `use_ssl`; `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` grava `tls_allow_invalid_server_cert` com o contrário do valor dado.
  - `libmariadb/mariadb_lib.c:3995-3999`: a impressão fixada (`MARIADB_OPT_TLS_PEER_FP`) liga `use_ssl` sozinha.
  - `plugins/auth/my_auth.c:314-319`: com a verificação ligada (o padrão da 3.4) ou com impressão fixada, `use_ssl` é ligado sozinho.
  - `plugins/auth/my_auth.c:343-354`: com `use_ssl` ligado e um servidor que não anuncia TLS, a recusa "SSL is required, but the server does not support it" só sai com a verificação ligada ou com impressão fixada (`tls_fp` ou `tls_fp_list`).
  - `plugins/auth/my_auth.c:357-360`: sem nenhum dos dois, o cliente tira o pedido de TLS e a sessão segue em texto claro.
  - `plugins/auth/caching_sha2_pw.c:70-76` e `:411-414`: o `caching_sha2_password` trata `use_ssl` ligado como conexão segura e manda a senha em claro; `:310-336`: sem TLS e sem arquivo de chave, pede a chave RSA ao servidor e cifra a senha para ela; `:325-336`: se o arquivo de chave não puder ser lido, também pede ao servidor.
  - `libmariadb/mariadb_lib.c:749-754` e `:1467`: a variável `MARIADB_TLS_DISABLE_PEER_VERIFICATION` é lida só no `mysql_init` e só pré-carrega `tls_allow_invalid_server_cert`; a opção gravada depois a sobrescreve (`:3775-3776`).
  - `libmariadb/mariadb_lib.c:1999`, `:2180-2187` e `:2533-2545`: o `mysql_real_connect` copia a senha para a memória comum (`strdup`), e a cópia é liberada sem zerar, no `mysql_close` e na falha da conexão.
- FATO medido em 19/11/2026, nos 16 pares das 4 distros (fedora45, ubuntu26.04, mint22.3 e manjaro):
  - sem nenhuma opção de TLS, contra o MariaDB 10.11 sem TLS: recusa, erro 2026, "SSL is required";
  - `MYSQL_OPT_SSL_ENFORCE`=1 com `MYSQL_OPT_SSL_VERIFY_SERVER_CERT`=0, contra o MariaDB 10.11 sem TLS: sessão aberta sem TLS (sab3);
  - `MYSQL_OPT_SSL_ENFORCE`=0 sem desligar a verificação: a sessão sobe com TLS nos servidores que oferecem TLS (sab1); só a verificação desligada faz a conexão sem TLS;
  - impressão fixada com a verificação ligada: abre com a impressão certa; recusa, erro 2026, com a impressão trocada; recusa, "SSL is required", no servidor sem TLS.
- Decisões do líder de 20/11/2026, 01:43, por AskUserQuestion: "Dois modos por tipo de banco (Recomendado)" e "Confirmar os três (Recomendado)" (a E4, a E8 e o residual de quem ocupa a porta).
- INFERÊNCIA do CISO: com ENFORCE=1 e a verificação desligada, quem está no caminho pode tirar o anúncio de TLS do servidor, e o cliente desce para texto claro sem erro. O nome "enforce" engana: a opção não obriga TLS.

## Decisão

1. O motor `src/foundation/sql` é o único lugar que fala com o Connector/C (o único `mysql_real_connect` fica em `src/foundation/sql/conexao.cpp`) e oferece os dois modos da E2 como mecanismo, sem conhecer a família do servidor. O adaptador de persistência (`src/mid/persistencia`, átomo `politica_conexao`) escolhe o modo pela família declarada pelo script. Nenhum código usa `MYSQL_OPT_SSL_ENFORCE` como guarda de TLS. A recusa de texto claro vem da impressão fixada com a verificação ligada, que é o modo fixado da E2. Nenhum código do adaptador desliga `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` fora do modo aberto da E2. No modo aberto, a verificação desligada vai junto com `MYSQL_OPT_SSL_ENFORCE`=0, sem `ssl_*` e sem impressão, e com lista restrita de plugins sem `caching_sha2_password` nem `sha256_password`. Com a verificação desligada e sem impressão, a sessão desce para texto claro (`my_auth.c:343-360`); por isso o modo vem do script e nunca do anúncio do servidor. A Foundation confere sozinha essas invariantes, sem depender do Mid.
2. Modos por família (decisão do líder de 20/11/2026, 01:43): MySQL no modo fixado, com a impressão do certificado e a lista `caching_sha2_password`; MariaDB no modo aberto, com a lista `mysql_native_password` e a senha longa sorteada pelo app; MariaDB com `require_secure_transport=ON` no modo fixado (NL-I.339). A impressão é a do certificado, nunca a chave RSA fixada (E4). O MariaDB 11.4+ vai ao modo aberto, como o 10.11 (E8).
3. O teste do motor e o do adaptador levam, contra um servidor sem TLS (classe Nenhum da matriz de integração), a mesma prova de recusa do modo fixado do teste de integração: erro 2026, "SSL is required" na mensagem, e nenhuma sessão aberta.
4. O produto não usa nem apaga a variável `MARIADB_TLS_DISABLE_PEER_VERIFICATION`: o modo é gravado depois do `mysql_init`, o que anula o efeito dela. Se a prova mostrar sessão aberta com a impressão trocada e a variável presente, o motor passa a recusar conectar com a variável presente.
5. Logo depois de uma conexão aberta, o motor zera o conteúdo da cópia da senha feita pelo conector (`MYSQL::passwd`), com `explicit_bzero`, sem mexer no ponteiro. Isso é seguro porque a reconexão automática fica desligada e o produto não usa `mysql_change_user`.
6. Este registro vale para o Connector/C 3.4.11. Versão nova do conector repete a leitura das linhas citadas antes de entrar.

## Como se verifica

- Hoje (19/11/2026): `src/` e `apps/` não chamam o conector (`git grep` de `mysql_`, `MYSQL_OPT` e `MARIADB_OPT` vazio em 8d7dc5c).
- Item 1: na fatia que criar o adaptador, um portão que reprova, em `src/` e `apps/`, `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` fora dos dois arquivos de modo (fixado e aberto), `MYSQL_OPT_SSL_ENFORCE` fora do arquivo do modo aberto (onde aparece uma vez só) e qualquer `MARIADB_TLS_DISABLE_PEER_VERIFICATION`, com contagem impressa e visto vermelho antes de valer (L-36). No arquivo do modo aberto, `MYSQL_OPT_SSL_ENFORCE` só existe com valor 0: um mutante com valor 1 tem de morrer no teste de integração da classe que oferece TLS e vai ao modo aberto.
- Item 2: o teste nomeado na fatia do adaptador, visto vermelho antes (sabotagem: tirar a impressão fixada e desligar `MYSQL_OPT_SSL_VERIFY_SERVER_CERT` (com ou sem `MYSQL_OPT_SSL_ENFORCE`=1, como a sab3 da NL-3.1h), e ver a sessão abrir sem TLS no servidor sem TLS).
- Item 3: a versão do conector está no manifesto Flatpak e em `tools/conteiner/modulo_mariadb.sh`; a troca de versão cita esta ADR no commit.
- Item 4: o teste de integração com a impressão trocada e `MARIADB_TLS_DISABLE_PEER_VERIFICATION=1` no ambiente do processo recusa a conexão, nos 16 pares.
- Item 5: o teste `senhaDoConectorZeradaDepoisDeConectar` lê todos os bytes de `MYSQL::passwd` depois de conectar, no tamanho da senha do par, e espera que nenhum seja diferente de 0, e a mesma conexão ainda roda uma consulta preparada; visto vermelho tirando a zeragem. A leitura de todos os bytes é da NL-3.1l (parecer do CISO, R4), porque a leitura só do primeiro byte aprovava uma zeragem que parasse nele.

## Consequências

- O ENFORCE sai do vocabulário de segurança do produto: ninguém presume TLS pelo nome da opção.
- O MariaDB no modo aberto tem sessão sem TLS. Residual aceito pelo líder em 20/11/2026, 01:43: com o servidor parado, quem ocupar a porta pode tentar quebrar a senha do banco fora da máquina; a defesa é a senha longa sorteada pelo app.
- Residual a ratificar pelo líder (L-15): a cópia da senha que o conector libera sem zerar quando a conexão falha, o vetor de pilha do `caching_sha2_password` e a janela do aperto de mão ficam na memória comum do processo, e podem aparecer num despejo de memória feito durante a conexão.
