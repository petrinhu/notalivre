# K2 do CLO (Cláudio): libsodium, cliente MariaDB e plugin de banco do Qt

Data: 04/11/2026. Nada foi editado no repositório, nada foi baixado, compilado ou instalado.

**Natureza:** orientação técnica, sem caráter vinculante. O que exige advogado humano está marcado na seção 7.

**Rótulos (L-18):** FATO cita arquivo:linha ou página aberta nesta tarefa. INFERÊNCIA é conclusão minha. NÃO VERIFICADO é o que não conferi. As páginas externas foram lidas por uma ferramenta que devolve trechos curtos e resumo. O que está entre aspas é trecho devolvido por ela; o resto é paráfrase dela, sem o texto integral.

**Frase-guarda (L-21), verbatim:** "Uso ou contagem de uso no repositório de um consumidor identificado nunca corta escopo, desenho ou qualidade de um produto feito para distribuição." Consumidor: MEI, autônomos e pequenas empresas do Simples Nacional, com distribuição a terceiros. A exclusão proposta na seção 3 vem do texto da licença, não de contagem de uso.

## 1. Vereditos

| Biblioteca | Licença | Exceção | Veredito para GPL-3.0-or-later em Flatpak |
|---|---|---|---|
| libsodium | ISC. A parte de Argon2 lida é CC0 1.0 | Nenhuma lida | **Aprovada** |
| Plugin de banco do Qt (QMYSQL e QMARIADB) | `LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only` | Nenhuma (sem `WITH`) | **Aprovado**, pela opção GPL-3.0-only |
| Connector/C, biblioteca `libmariadb` | Misto, não é só "LGPL-2.1": seção 3 | Nenhuma lida | **Aprovada com condições** |
| Connector/C, `plugins/auth/parsec.c` | **Só GPL versão 2** | Nenhuma lida | **Fora do pacote** |

**Nada impede o K2.** A condição que evita o impedimento é compilar sem o parsec. Ela vale desde o commit que puser o módulo no manifesto, não só na NL-9.1.

**Linha pronta para o relatório da fatia:**

> K2 (Cláudio, 04/11/2026, orientação não vinculante): libsodium ISC, aprovada; plugin QMYSQL/QMARIADB do Qt aprovado pela opção GPL-3.0-only; Connector/C (LGPL com trechos de outras licenças) aprovado com condições: compilar com `-DCLIENT_PLUGIN_PARSEC=OFF`, porque `plugins/auth/parsec.c` é só GPL 2, fixar tag com SHA-256 e repetir a varredura nela, e levar os avisos a `share/licenses` e à tela Sobre; a leitura da licença PHP pela versão 4 (BSD-3-Clause) e o caso parsec vão a advogado humano antes da `v1.0.0.0`.

## 2. libsodium

**FATOS**

| Fato | Fonte aberta |
|---|---|
| Título "ISC License". Trecho: "Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted,". Exige manter o aviso de copyright e o de permissão em todas as cópias (paráfrase). Copyright 2013 a 2026, Frank Denis. Igual em `master` e `stable` | https://raw.githubusercontent.com/jedisct1/libsodium/master/LICENSE e o mesmo caminho em `stable` |
| A implementação de Argon2 lida diz: "This work is licensed under a Creative Commons CC0 1.0 License/Waiver." | https://raw.githubusercontent.com/jedisct1/libsodium/master/src/libsodium/crypto_pwhash/argon2/argon2.c |
| `AUTHORS` lista projetistas e implementadores, sem falar de licença | https://raw.githubusercontent.com/jedisct1/libsodium/master/AUTHORS |
| FSF sobre a ISC: "It is a free software license, and compatible with the GNU GPL." Sobre a CC0: "Both public domain works and the lax license provided by CC0 are compatible with the GNU GPL." | https://www.gnu.org/licenses/license-list.en.html (âncora `#ISC`, entrada CC0) |

**INFERÊNCIA:** compatível, em ligação dinâmica ou estática. A linha plano:133 pode trocar INFERÊNCIA por FATO. O uso ampliado previsto em `TODO.md:79` (SenhaSegura) não muda a licença.

**NÃO VERIFICADO:** os demais arquivos da árvore da libsodium, um a um.

## 3. Connector/C: o texto da licença

### 3.1 O que cada fonte diz (FATOS, ramo `3.4`, sem tag)

| Onde | O que diz |
|---|---|
| `COPYING.LIB` (raiz, único arquivo de licença na raiz) | "GNU LESSER GENERAL PUBLIC LICENSE", "Version 2.1, February 1999". Igual no ramo `3.1` |
| API do GitHub | `lgpl-2.1`, ramo padrão `3.4` |
| `README` | "This is LGPL MariaDB client library"; "This product includes PHP software, freely available from <http://www.php.net/software/>" |
| FAQ da MariaDB | Bibliotecas cliente "are distributed under the LGPL license, version 2.1 or later" |
| Maior parte dos arquivos, por exemplo `libmariadb/mariadb_lib.c` e `include/mysql.h` | "GNU Library General Public License", "version 2 of the License, or (at your option) any later version". É o nome antigo, versão 2, não 2.1 |
| `libmariadb/ma_context.c` | LGPL 2.1 ou posterior (paráfrase da ferramenta) |
| `libmariadb/ma_dtoa.c` | Library GPL "version 2", sem "or later" (a ferramenta confirmou a ausência), mais aviso permissivo de Lucent e David M. Gay |
| `mariadb_stmt.c`, `ma_stmt_codec.c`, `ma_charset.c`, `ma_loaddata.c` | Cabeçalho Library GPL 2 ou posterior **e** bloco original "subject to version 3.01 of the PHP license", Copyright The PHP Group, 2006 a 2011 |
| `libmariadb/mariadb_lib.c` | Além do cabeçalho LGPL: "Part of this code includes code from the PHP project" |
| `libmariadb/ma_password.c` (no meio do arquivo, em `ma_scramble_41`) | "License: PHP License 3.0", código baseado no mysqlnd do PHP |
| `mariadb_dyncol.c` | Estilo BSD (só entra com `WITH_DYNCOL`) |
| `plugins/auth/auth_gssapi_client.c`, `gssapi_client.c`, `gssapi_errmsg.c` | BSD de 2 cláusulas |
| `plugins/io/remote_io.c` | BSD de 3 cláusulas, com Simtec Electronics, embora o metadado do plugin diga "LGPL" |
| **`plugins/auth/parsec.c`** | Copyright 2024, MariaDB plc. GNU General Public License, "version 2 of the License." A ferramenta confirmou a ausência de "or (at your option) any later version" |
| `plugins/auth/my_auth.c` | Sem cabeçalho; só o metadado "LGPL" |
| `plugins/auth/ref10/fe_0.c` (amostra) | Sem licença nem copyright |

Endereços: `https://raw.githubusercontent.com/mariadb-corporation/mariadb-connector-c/3.4/<caminho>`, e https://mariadb.com/docs/general-resources/community/community/faq/licensing-questions/licensing-faq.

**Fontes secundárias, que só corroboram:** o `debian/copyright` da MariaDB lista `libmariadb/*` como LGPL-2+ com The PHP Group no copyright, e `ma_dtoa.*` como LGPL-2 (https://salsa.debian.org/mariadb-team/mariadb-server/-/raw/debian/latest/debian/copyright). Uma auditoria de terceiro na 3.4.8 chegou a `LGPL-2.1-or-later AND PHP-3.01` (https://github.com/cy-pca/cvcpkg/pull/198). Um pacote de terceiro exclui o parsec pelo mesmo cabeçalho (https://github.com/bonSQL/bonsql/releases/tag/v2026.10.9). Nenhuma delas é posição da MariaDB.

**Por que os arquivos divergem do `COPYING.LIB`:** FATO: o `README` diz "This code is based on the LGPL libmysql client library from MySQL 3.23 and PHP's mysqlnd extension." INFERÊNCIA: os cabeçalhos "Library GPL 2" vêm dessa herança, e o `COPYING.LIB` em 2.1 convive com eles sem conflito, porque "ou posterior" alcança a 2.1. NÃO VERIFICADO: o histórico de commits do arquivo. A divergência não muda a compatibilidade (seção 3.4).

**Correção a plano:138:** "LGPL-2.1" é simplificação. INFERÊNCIA: a leitura correta é "LGPL 2 ou posterior na maior parte, com `COPYING.LIB` em 2.1, trechos só LGPL 2, BSD e PHP, e um plugin só GPL 2".

### 3.2 Parsec: o único texto incompatível que achei

- FATO: "Please note that GPLv2 is, by itself, not compatible with GPLv3." (https://www.gnu.org/licenses/license-list.en.html, âncora `#GPLv2`).
- FATO: `plugins/auth/CMakeLists.txt` registra o parsec com padrão DYNAMIC e configurações DYNAMIC, STATIC e OFF. Sem ação, o build o gera.
- FATO: `cmake/plugins.cmake` usa `string(TOUPPER ${CC_PLUGIN_TARGET} cc_plugin)` e a variável `CLIENT_PLUGIN_${cc_plugin}`; com OFF, o plugin não é compilado nem instalado (paráfrase). A variável é `CLIENT_PLUGIN_PARSEC`, em maiúsculas. **Não medido em build** (proibido nesta tarefa).
- FATO: o `CMakeLists.txt` de autenticação do ramo `3.3` não cita parsec.
- INFERÊNCIA: como `.so` carregado pela `libmariadb` no mesmo processo do app, ele tende a formar obra combinada com código só GPL 3, que a GPL 2 não permite. Tratar como mera agregação seria aposta.
- FATO: a FAQ da MariaDB, sobre módulos GPL carregados por um framework independente de banco, diz: "any GPL licensed module loaded by the framework will not affect the application." INFERÊNCIA: é a posição da titular num caso próximo, não neste. O parsec é carregado pela `libmariadb`, e a página não cita plugins do conector nem app GPL 3. Fica como argumento para o advogado. A recomendação é não distribuir, porque excluir custa pouco.

**Caminho proposto (recomendação ao CTO e ao main, sem decisão minha):**

1. `-DCLIENT_PLUGIN_PARSEC=OFF` no módulo do manifesto, com conferência de que nenhum arquivo parsec ficou no bundle. Essa conferência tem de ser vista reprovando uma vez (L-36).
2. Efeito para o usuário (INFERÊNCIA): conta de banco configurada para autenticar por parsec não conecta. FATO: `caching_sha2_pw.c` e `sha256_pw.c` são LGPL e seguem disponíveis. NL-3.6 deve conferir que o script cria contas com outro método. NÃO VERIFICADO: método padrão do MariaDB 11.8 e do MySQL 8.4, que é a pesquisa de abertura da Onda 3 (plano:689).
3. Alternativas, se o CTO recusar: fixar uma versão anterior ao parsec (`3.3`; NÃO VERIFICADO se ainda recebe correção) ou perguntar à MariaDB se o cabeçalho é erro, já que os demais plugins de autenticação lidos são LGPL (INFERÊNCIA: pode ser descuido, mas não se publica contando com isso).

### 3.3 Trechos sob licença PHP: compatíveis por uma cadeia de quatro elos

1. FATO: "It is incompatible with the GNU GPL because it includes strong restrictions" sobre a PHP 3.01 (FSF, âncora `#PHP-3.01`). Sozinha, ela seria problema.
2. FATO: cláusula 5 da 3.01: "You may also choose to use such covered code under the terms of any subsequent version of the license published by the PHP Group." (https://www.php.net/license/3_01.txt).
3. FATO: "At your option, you may choose to use any earlier version of PHP software under the terms of the PHP License, version 4", com SPDX `BSD-3-Clause` (https://www.php.net/license/index.php). RFC aceita por 51 a 0, com 2 abstenções (https://wiki.php.net/rfc/php_license_update).
4. FATO: BSD modificada: "compatible with the GNU GPL" (FSF, âncora `#ModifiedBSD`).

**INFERÊNCIA:** a opção alcança o código do mysqlnd levado ao Connector/C, inclusive o trecho marcado "PHP License 3.0" (`ma_password.c`), que li só como "any earlier version". Esse é o elo que vai ao advogado (seção 7). Sem ele, o caso seria incompatibilidade em código essencial da biblioteca, não em plugin removível.

**Obrigação:** copyright e aviso da PHP Group com as condições da BSD. Recomendo manter também a frase "This product includes PHP software...", como o `README` faz: custa uma linha e cobre a leitura contrária.

### 3.4 LGPL e GPL-3

- FATO: LGPL 2.1: "It is compatible with GPLv2 and GPLv3." (FSF, âncora `#LGPLv2.1`).
- FATO: `COPYING.LIB` e o texto LGPL 2 (https://raw.githubusercontent.com/spdx/license-list-data/main/text/LGPL-2.0-only.txt), seção 3: "You may opt to apply the terms of the ordinary GNU General Public License instead of this License"; "If a newer version than version 2 of the ordinary GNU General Public License has appeared" (segue a permissão de indicar essa versão, em paráfrase).
- INFERÊNCIA: `ma_dtoa.c`, o trecho só LGPL 2, também é compatível: a biblioteca é usada como LGPL, e a seção 3 é a saída para GPL 3 se precisar.
- NÃO VERIFICADO: a tabela de compatibilidade do FAQ da FSF. O servidor recusou (HTTP 429) todas as tentativas; usei a lista de licenças.

**Preferência do CTO por `libmariadb` em vez de `libmysqlclient`: confirmada, por simplicidade.** FATO: o MySQL elege "only" a GPL 2 e dá ao cliente C a Universal FOSS Exception 1.0, cujo teste é licença aprovada pela OSI ou classificada como livre pela FSF (https://raw.githubusercontent.com/mysql/mysql-server/trunk/LICENSE; https://oss.oracle.com/licenses/universal-foss-exception/). INFERÊNCIA: daria certo pela exceção, mas o caminho dependeria só dela.

## 4. Plugin de banco do Qt

- FATO: `// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`, em `qsql_mysql.cpp` nas tags 6.12.0 e `dev`, e em `main.cpp` na 6.11.3 (https://raw.githubusercontent.com/qt/qtbase/6.12.0/src/plugins/sqldrivers/mysql/qsql_mysql.cpp; https://raw.githubusercontent.com/qt/qtbase/6.11.3/src/plugins/sqldrivers/mysql/main.cpp). É a mesma expressão do núcleo do Qt, lida na nota anterior.
- FATO: um só plugin atende MySQL e MariaDB; só o de SQLite vem nos binários do Qt; a página pede "verify the client library's license compatibility with the LGPL" (https://doc.qt.io/qt-6/sql-driver.html).
- INFERÊNCIA: compatível pela opção GPL-3.0-only, igual à P5 e sem amarra nova. A cautela do Qt é atendida porque a `libmariadb` é LGPL, com a condição do parsec.

## 5. Ligação dinâmica e pacote Flatpak

**FATO medido nesta tarefa:** em `~/.local/share/flatpak/runtime/org.kde.Platform/x86_64/6.11/` e no `org.kde.Sdk` 6.11, o único driver é `files/lib/plugins/sqldrivers/libqsqlite.so`. Não há `libqsqlmysql`, `libmariadb`, `libmysqlclient` nem `libsodium`. Controle positivo: a mesma busca na Platform acha `libQt6NetworkAuth.so.6.11.3` e `libcrypto.so.3`. Isso resolve `docs/pesquisa/flatpak.md:33` para a 6.11. **Runtime 6.12 não medido.**

**Cadeia (INFERÊNCIA a partir de plano:133, plano:138 e dos arquivos acima):** o app liga a libsodium; o QtSql carrega o plugin; o plugin liga a `libmariadb`; a `libmariadb` carrega os plugins de autenticação.

**Consequência:** diferente do QtNetworkAuth, que vem do runtime, **essas três peças entram como módulos do NotaLivre** (plano:796). Quem distribui os binários é o projeto, e as obrigações são diretas. Para essas três peças, a dúvida do runtime como biblioteca de sistema, levantada na nota anterior, seção 1.3, não se aplica.

**Ponto com o K3 (sem reabri-lo):** compilar o plugin a partir do `qtbase` é distribuir uma pequena parte do Qt, mesmo sem o Qt embutido. Os avisos do Qt do item K3b passam a valer para ela. Ela segue a versão do runtime, não a 6.12 (INFERÊNCIA).

## 6. Obrigações ao distribuir

| Obrigação | Fonte | Como cumprir |
|---|---|---|
| Aviso de copyright e de permissão da ISC em todas as cópias | `LICENSE` da libsodium | Em `share/licenses/$FLATPAK_ID` e na tela de licenças |
| Aviso visível de que a biblioteca é usada e está sob a LGPL, com cópia da licença: "You must give prominent notice with each copy of the work that the Library is used in it" | `COPYING.LIB`, seção 6 | Idem |
| Se o programa mostra copyright ao rodar ("If the work during execution displays copyright notices"), incluir o da biblioteca e "a reference directing the user to the copy of this License" | `COPYING.LIB`, seção 6 | **Reforça o item da tela "Sobre"** apontado na nota anterior, seção 1.3 |
| Permitir modificação para uso próprio e engenharia reversa para depuração; mecanismo de biblioteca compartilhada: "suitable shared library mechanism" | `COPYING.LIB`, seção 6 e 6(b) | INFERÊNCIA: a GPL do app já permite. Manter `libmariadb` como `.so`, que é o caminho mais simples |
| Texto da GPL 3 e do Qt: LGPL 3 ou GPL 3, conforme a opção | SPDX acima | Declarar GPL-3.0-only para o plugin |
| Avisos da PHP Group (BSD), Lucent/David M. Gay, Monty Program e Oleksandr Byelkin (dyncol, se ligado), Simtec e autores do GSSAPI, se os plugins forem | Cabeçalhos da seção 3.1 | Arquivo de avisos. Recomendo desligar `remote_io` e GSSAPI se o CTO não precisar, para encurtar a lista |
| Fonte completa do que se distribui, com "clear directions next to the object code", mantida "for as long as needed" | GPL 3, seção 6(d), lida na nota anterior | `sources` com URL e SHA-256 **e** cópia sob controle do projeto. INFERÊNCIA: a URL do terceiro sozinha é frágil. FATO: `--bundle-sources` cria o runtime `.Sources` (https://docs.flatpak.org/en/latest/flatpak-builder-command-reference.html) |
| Arquivos de licença de cada módulo em `$FLATPAK_DEST/share/licenses/$FLATPAK_ID` | `docs/juridico/k2-redistribuicao-xsd-e-tabelas.md:75`, requisito do Flathub | Propriedade `license-files` do módulo. FATO: "Array of paths to LICENSE files of the module."; a referência não descreve cópia automática, logo declarar explicitamente |
| MetaInfo | k2:75 e nota anterior, seção 2.2 | INFERÊNCIA: a licença do app segue GPL-3.0-or-later, com programa pronto em GPL 3. A forma de citar terceiros fica para o K3, na NL-9.1 |

**Tela de licença.** Sem mudar a recomendação anterior, ela passa a listar: libsodium, Connector/C, plugin do Qt e PHP Group, com o texto de cada licença acessível. Na CLI, a mesma lista numa saída de licença. Só o que estiver no bundle entra.

**Artefato do CI (`.github/workflows/ci.yml`, citado na nota anterior):** INFERÊNCIA: assim que o módulo entrar no manifesto, esse bundle já leva estes binários. Por isso a condição do parsec vale desde esse commit. A fonte do mesmo commit está no repositório, mas não a das dependências, salvo pelas URLs do manifesto.

## 7. NÃO VERIFICADO e o que vai a advogado humano

**Advogado humano, antes da `v1.0.0.0` (não trava a Onda 3):**

1. A escolha da versão 4 da licença PHP para código levado a uma biblioteca de terceiro, sob cabeçalho LGPL (seção 3.3). É o ponto de maior peso, porque esse código não dá para desligar.
2. O parsec como obra combinada ou agregação (seção 3.2), se alguém quiser distribuí-lo.

**NÃO VERIFICADO:**

- **Avisos no meio dos arquivos.** Li só cabeçalhos e pedi "licença" à ferramenta. O caso `ma_password.c` mostra o risco: o cabeçalho dizia só LGPL, e o trecho PHP está no meio do arquivo e a primeira leitura não o viu. A varredura do item abaixo cobre isso.
- **Tag.** Li o ramo móvel `3.4`, não uma tag. **Ao escolher a versão, repetir no tarball exato (SHA-256):** busca por `License`, `PHP` e `GPL` em toda a árvore, com contagem de arquivos varridos, mais `COPYING.LIB`, `parsec.c`, `ma_dtoa.c` e os arquivos PHP. Quem executa é o agente da fatia.
- **Cobertura.** Cabeçalho lido nos 32 arquivos compilados em Linux que a lista da ferramenta mostrou (núcleo, com `plugins/auth/my_auth.c`, mais `secure/openssl.c`, `openssl_crypt.c` e `mariadb_dyncol.c`) e em outros 16 arquivos de plugin. Não lidos: demais `include/`, `plugins/auth/common.h` e `ed25519_common.h`, resto de `ref10/` (a auditoria de terceiro diz domínio público; não confirmei a origem), `trace_example.c`, variantes GnuTLS e Windows, `win/packaging/license.rtf`, `client/`, `unittest/`, `external/zlib` (recomendo `WITH_EXTERNAL_ZLIB`; FATO: a opção existe em `libmariadb/CMakeLists.txt`).
- **Histórico.** Li `COPYING.LIB` só nos ramos `3.4` e `3.1`.
- **Texto integral** das licenças: a ferramenta devolve trechos.
- **`dev`, tag 6.12.0 e 6.11.3 do Qt:** só os arquivos citados. Licença de terceiros dentro do `qtbase`, não lida.
- **Oferta de fonte pelo Flathub** e tabela do FAQ da FSF.
- **Runtime 6.12**, efeito real de `-DCLIENT_PLUGIN_PARSEC=OFF` e se os binários do Qt pelo aqt trazem o plugin MySQL.

## 8. Pedidos ao main (L-10; eu não disparo)

| Agente | Para quê | Quando |
|---|---|---|
| `technical-writer` | plano:133 (ISC, FATO), plano:138 (licença como na seção 3.1, condição do parsec, ausência medida na 6.11), `docs/pesquisa/flatpak.md:33`, e registrar este K2 em `docs/juridico/` | Na fatia que trouxer a dependência |
| Executor da fatia ou `devops-sre` | Módulos no manifesto com a condição do parsec, `license-files` e conferência sem parsec no bundle; varredura na tag | Commit que puser o módulo no manifesto |
| Caetano (CTO) | Aceitar ou recusar a exclusão do parsec e o desligamento de `remote_io` e GSSAPI; NL-3.6 sem conta parsec | Antes dessa fatia |
| INBOX do `TODO.md`, pelo main | Consulta ao advogado sobre a seção 7, antes da `v1.0.0.0` | Quando o main julgar |

Fontes: todas com endereço nas seções acima.
