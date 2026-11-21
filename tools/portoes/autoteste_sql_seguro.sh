#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão sql_seguro (NL-3.1a, ordem v1 seção 1.5 e emenda v1b seção 3, L-36, L-43).
#
# Cada caso monta uma árvore de fixture (src/ e apps/) numa pasta do scratch, sabota o motor
# `src/foundation/sql` do jeito que o caso pede e exige rc EXATO e uma regex sobre a linha de
# saída do portão. Os 14 casos ficam fixados aqui antes de o portão existir (L-43).
#
# Casos: c1 a c11 são vermelhos, um por regra (S1 a S8); c12 é a árvore vazia (piso, L-36);
# c13 e c14 são verdes (falso positivo e árvore completa).
#
# Uso: autoteste_sql_seguro.sh
#   PORTAO (variável de ambiente): troca o portão testado (sabotagem em cópia fora da árvore, L-27).
# Pastas de trabalho em ${TMPDIR:-/var/tmp}, apagadas ao fim (L-53: só o próprio scratch).
# Saída final: `autoteste_sql_seguro: casos=<n> falharam=<n>`; sai com 1 se algum caso falhar.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORTAO="${PORTAO:-$SCRIPT_DIR/sql_seguro.py}"

TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/nl-autoteste-sql-seguro.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT

TOTAL=0
FALHAS=0

# Árvore base do motor, válida para todas as regras: 12 .cpp e os cabeçalhos do motor nos lugares certos.
montar_base()
{
    local d="$1"
    local s="$d/src/foundation/sql"
    mkdir -p "$s/include/nl/foundation/sql" "$s/privado/nl/foundation/sql" "$d/apps/cli" || exit 1

    printf '%s\n' '#pragma once' '// consulta' > "$s/include/nl/foundation/sql/consulta.h"
    printf '%s\n' '#pragma once' '// parametro' > "$s/include/nl/foundation/sql/parametro.h"
    printf '%s\n' '#pragma once' '// dados de conexao' > "$s/include/nl/foundation/sql/dados_conexao.h"
    printf '%s\n' '#pragma once' '// executor' > "$s/include/nl/foundation/sql/executor.h"
    printf '%s\n' '#pragma once' '// validacao' > "$s/privado/nl/foundation/sql/validacao.h"
    printf '%s\n' '#pragma once' '// vinculos' > "$s/privado/nl/foundation/sql/vinculos.h"
    printf '%s\n' '#pragma once' '// preparado' > "$s/privado/nl/foundation/sql/preparado.h"
    printf '%s\n' '#pragma once' '// leitura' > "$s/privado/nl/foundation/sql/leitura.h"
    printf '%s\n' '#pragma once' '// costura' > "$s/privado/nl/foundation/sql/costura_conexao.h"

    local cpp
    for cpp in parametro validacao vinculos preparado leitura transacao erros executor; do
        printf '%s\n' "// $cpp" > "$s/$cpp.cpp"
    done
    # S2: cada uma das 6 opções aparece exatamente uma vez, e só aqui.
    printf '%s\n' '// opcoes de sessao' 'MYSQL_OPT_LOCAL_INFILE' 'MYSQL_OPT_RECONNECT' 'MYSQL_INIT_COMMAND' \
        'MARIADB_OPT_RESTRICTED_AUTH' 'MYSQL_OPT_PROTOCOL' 'MYSQL_SET_CHARSET_NAME' > "$s/opcoes_sessao.cpp"
    # S3 e S4 no modo aberto: verificação desligada e ENFORCE exatamente uma vez.
    printf '%s\n' '// modo aberto' 'MYSQL_OPT_SSL_VERIFY_SERVER_CERT' 'MYSQL_OPT_SSL_ENFORCE' > "$s/modo_aberto.cpp"
    # S3 e S5 no modo fixado: verificação e impressão; sem ENFORCE.
    printf '%s\n' '// modo fixado' 'MYSQL_OPT_SSL_VERIFY_SERVER_CERT' 'MARIADB_OPT_TLS_PEER_FP' > "$s/modo_fixado.cpp"
    # S6 e S7: o único mysql_real_connect, e a include do conector (permitida só aqui).
    printf '%s\n' '#include <mysql.h>' '// conexao' 'mysql_real_connect' > "$s/conexao.cpp"
}

# Acrescenta uma linha de sabotagem a um arquivo da fixture (ARQ relativo à raiz da fixture).
acrescentar()
{
    local nome="$1" arq="$2" linha="$3"
    printf '%s\n' "$linha" >> "$TMP/$nome/$arq" || { echo "ERRO: sabotagem $nome em $arq"; exit 1; }
}

# Confere um caso: rc exato, linha de saída do portão presente, regex ERE casando a saída.
controle()
{
    local nome="$1" rc_esperado="$2" regex="$3"
    local saida="$TMP/saida-$nome.txt" rc ok=1

    TOTAL=$((TOTAL + 1))
    python3 -I "$PORTAO" "$TMP/$nome" > "$saida" 2>&1
    rc=$?
    echo "caso $nome: rc=$rc (esperado $rc_esperado)"
    sed 's/^/    | /' "$saida"

    [ "$rc" = "$rc_esperado" ] || ok=0
    grep -Eq -- "$regex" "$saida" || ok=0

    if [ "$ok" = 1 ]; then
        echo "caso $nome: VERDE"
    else
        echo "caso $nome: VERMELHO"
        FALHAS=$((FALHAS + 1))
    fi
}

# Início de todo caso que tem árvore base.
novo_caso()
{
    local nome="$1"
    mkdir -p "$TMP/$nome" || { echo "ERRO: pasta do caso $nome"; exit 1; }
    montar_base "$TMP/$nome"
}

if [ ! -f "$PORTAO" ]; then
    echo "ABORTA: portão ausente em $PORTAO (os casos abaixo reprovam por isso)"
fi
echo "portao testado: $PORTAO"
echo "scratch: $TMP"

ERE_REPROVA_RAIZ='^sql_seguro: arquivos=[1-9][0-9]* '

# --- S1: texto proibido em todo src e apps (MULTI_STATEMENTS e mysql_change_user) ---
novo_caso c1; acrescentar c1 src/foundation/sql/executor.cpp 'MARIADB_OPT_MULTI_STATEMENTS'
controle c1 1 "${ERE_REPROVA_RAIZ}s1=1 .*resultado=REPROVA\$"

novo_caso c2; acrescentar c2 src/foundation/sql/executor.cpp 'mysql_change_user(conexao, usuario, senha, banco);'
controle c2 1 "${ERE_REPROVA_RAIZ}s1=1 .*resultado=REPROVA\$"

# --- S2: cada opção de sessão exatamente uma vez, e só em opcoes_sessao.cpp ---
novo_caso c3; acrescentar c3 src/foundation/sql/executor.cpp 'MYSQL_OPT_LOCAL_INFILE'
controle c3 1 "${ERE_REPROVA_RAIZ}.*s2_ok=5/6 .*resultado=REPROVA\$"

# --- S3: verificação do certificado só nos dois arquivos de modo ---
novo_caso c4; acrescentar c4 src/foundation/sql/executor.cpp 'MYSQL_OPT_SSL_VERIFY_SERVER_CERT'
controle c4 1 "${ERE_REPROVA_RAIZ}.*s3_ok=0 .*resultado=REPROVA\$"

# --- S4: MYSQL_OPT_SSL_ENFORCE exatamente uma vez, em modo_aberto.cpp ---
novo_caso c5; acrescentar c5 src/foundation/sql/modo_fixado.cpp 'MYSQL_OPT_SSL_ENFORCE'
controle c5 1 "${ERE_REPROVA_RAIZ}.*s4_ok=0 .*resultado=REPROVA\$"

novo_caso c6; acrescentar c6 src/foundation/sql/modo_aberto.cpp 'MYSQL_OPT_SSL_ENFORCE'
controle c6 1 "${ERE_REPROVA_RAIZ}.*s4_ok=0 .*resultado=REPROVA\$"

novo_caso c7; sed -i '/MYSQL_OPT_SSL_ENFORCE/d' "$TMP/c7/src/foundation/sql/modo_aberto.cpp" \
    || { echo "ERRO: sabotagem c7"; exit 1; }
controle c7 1 "${ERE_REPROVA_RAIZ}.*s4_ok=0 .*resultado=REPROVA\$"

# --- S5: impressão do certificado só em modo_fixado.cpp ---
novo_caso c8; acrescentar c8 src/foundation/sql/modo_aberto.cpp 'MARIADB_OPT_TLS_PEER_FP'
controle c8 1 "${ERE_REPROVA_RAIZ}.*s5_ok=0 .*resultado=REPROVA\$"

# --- S6: exatamente um mysql_real_connect, em conexao.cpp ---
novo_caso c9; acrescentar c9 src/foundation/sql/conexao.cpp 'mysql_real_connect'
controle c9 1 "${ERE_REPROVA_RAIZ}.*s6_ok=0 .*resultado=REPROVA\$"

# --- S7: include do conector só em src/foundation/sql/*.cpp e privado/ ---
novo_caso c10; acrescentar c10 src/foundation/sql/include/nl/foundation/sql/consulta.h '#include <mysql.h>'
controle c10 1 "${ERE_REPROVA_RAIZ}.*s7=1 .*resultado=REPROVA\$"

# --- S8: variável que desliga a verificação, zero em src e apps ---
novo_caso c11; acrescentar c11 src/foundation/sql/executor.cpp 'MARIADB_TLS_DISABLE_PEER_VERIFICATION'
controle c11 1 "${ERE_REPROVA_RAIZ}.*s8=1 .*resultado=REPROVA\$"

# --- Piso (L-36): árvore sem arquivo nenhum reprova, e não passa em silêncio ---
mkdir -p "$TMP/c12" || { echo "ERRO: pasta c12"; exit 1; }
controle c12 1 '^sql_seguro: arquivos=0 .*resultado=REPROVA$'

# --- Controles verdes: falso positivo (identificador e comentário) e árvore completa ---
novo_caso c13; acrescentar c13 src/foundation/sql/executor.cpp 'int forma_x = 0; // mariadb sem include'
controle c13 0 '^sql_seguro: arquivos=[1-9][0-9]* s1=0 s2_ok=6/6 s3_ok=1 s4_ok=1 s5_ok=1 s6_ok=1 s7=0 s8=0 resultado=PASSA$'

novo_caso c14
controle c14 0 '^sql_seguro: arquivos=[1-9][0-9]* s1=0 s2_ok=6/6 s3_ok=1 s4_ok=1 s5_ok=1 s6_ok=1 s7=0 s8=0 resultado=PASSA$'

echo "autoteste_sql_seguro: casos=$TOTAL falharam=$FALHAS"
[ "$FALHAS" = 0 ] || exit 1
exit 0
