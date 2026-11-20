#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da lista única de opções do Connector/C (NL-3.1h, V-c; L-35, L-36, L-17). A lista mora em modulo_mariadb.sh e
# no manifesto Flatpak, e este autoteste amarra os dois: o conjunto de opções -D tem de ser igual nos dois, e tem de trazer
# as 8 opções fixadas no passo 6.2 da ordem (v3.10). Vermelho antes do módulo: modulo_mariadb.sh e o manifesto ainda não têm a lista.
#   F1 modulo_mariadb.sh traz exatamente as 8 opções fixadas.
#   F2 manifesto e modulo_mariadb.sh têm o mesmo conjunto de opções -D.
#   F3 controle do comparador: uma opção trocada num manifesto de sandbox tem de ser vista como divergência.
# Não sobe contêiner. Uso: autoteste_flags_mariadb.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MODULO="$RAIZ/tools/conteiner/modulo_mariadb.sh"
MANIFESTO="$RAIZ/flatpak/io.github.petrinhu.notalivre.yml"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-flags.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Opções fixadas no passo 6.2 (v3.10: 8 opções; REMOTE_IO sai, o controle é -DWITH_CURL=OFF). Lista única: mudança aqui
# exige mudança no módulo e no manifesto no mesmo commit.
ESPERADAS="-DCMAKE_BUILD_TYPE=Release
-DWITH_SSL=OPENSSL
-DWITH_EXTERNAL_ZLIB=ON
-DWITH_UNIT_TESTS=OFF
-DWITH_CURL=OFF
-DWITH_DOCS=OFF
-DCLIENT_PLUGIN_PARSEC=OFF
-DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=OFF"

# Conjunto ordenado das opções -D de um arquivo; arquivo ausente sai com rc 1 (nunca conjunto vazio silencioso).
opcoes_de() {
    [ -f "$1" ] || return 1
    grep -oE -- '-D[A-Z_]+=[A-Za-z0-9_]+' "$1" | sort -u
}

# Bloco do módulo mariadb-connector-c no manifesto: do "- name: mariadb-connector-c" até o próximo "- name:".
bloco_mariadb() {
    awk '/^  - name: mariadb-connector-c$/ { ativo = 1; next } /^  - name: / { ativo = 0 } ativo' "$1"
}

# F1: o script traz exatamente as opções esperadas, nos dois sentidos (falta e sobra).
if conjunto_modulo="$(opcoes_de "$MODULO")"; then
    esperadas_ord="$(printf '%s\n' "$ESPERADAS" | sort -u)"
    faltam="$(comm -23 <(printf '%s\n' "$esperadas_ord") <(printf '%s\n' "$conjunto_modulo"))"
    sobram="$(comm -13 <(printf '%s\n' "$esperadas_ord") <(printf '%s\n' "$conjunto_modulo"))"
    if [ -z "$faltam" ] && [ -z "$sobram" ]; then ok "F1 modulo_mariadb.sh traz exatamente as 8 opções fixadas"
    else falha "F1 modulo_mariadb.sh: faltam [$(printf '%s' "$faltam" | tr '\n' ' ')] sobram [$(printf '%s' "$sobram" | tr '\n' ' ')]"; fi
else
    falha "F1 modulo_mariadb.sh ausente: sem lista de opções do Connector/C"
    conjunto_modulo=""
fi

# F2: o bloco do módulo no manifesto tem o mesmo conjunto de opções do script (o resto do manifesto fica de fora).
if [ -f "$MANIFESTO" ] && bloco_mariadb "$MANIFESTO" | grep -q . ; then
    conjunto_manifesto="$(bloco_mariadb "$MANIFESTO" | grep -oE -- '-D[A-Z_]+=[A-Za-z0-9_]+' | sort -u)"
    if [ -n "$conjunto_modulo" ] && [ "$conjunto_manifesto" = "$conjunto_modulo" ]; then
        ok "F2 bloco mariadb-connector-c do manifesto e modulo_mariadb.sh têm o mesmo conjunto de opções"
    else
        falha "F2 manifesto e modulo_mariadb.sh divergem, ou o script não tem lista"
    fi
else
    falha "F2 manifesto Flatpak ausente ou sem o módulo mariadb-connector-c"
fi

# F4: o bloco do manifesto tem o post-install que confere os plugins proibidos e o caching_sha2_password.so.
bloco="$(bloco_mariadb "$MANIFESTO" 2>/dev/null)"
if printf '%s\n' "$bloco" | grep -q 'post-install:' \
    && printf '%s\n' "$bloco" | grep -q 'parsec\.so' \
    && printf '%s\n' "$bloco" | grep -q 'auth_gssapi_client\.so' \
    && printf '%s\n' "$bloco" | grep -q 'remote_io\.so' \
    && printf '%s\n' "$bloco" | grep -q 'caching_sha2_password\.so'; then
    ok "F4 manifesto tem o post-install com os plugins proibidos e o caching_sha2_password.so"
else
    falha "F4 manifesto sem o post-install de conferência dos plugins"
fi

# F3: controle do comparador. Um manifesto de sandbox com WITH_SSL trocado por GNUTLS tem de dar divergência.
mkdir -p "$BASE/sandbox"
printf -- '-DWITH_SSL=OPENSSL\n-DWITH_SSL=GNUTLS\n' > "$BASE/sandbox/divergente.txt"
printf -- '-DWITH_SSL=OPENSSL\n' > "$BASE/sandbox/igual.txt"
ref="$(opcoes_de "$BASE/sandbox/igual.txt")"
div="$(opcoes_de "$BASE/sandbox/divergente.txt")"
if [ "$ref" != "$div" ]; then ok "F3 comparador vê a opção trocada como divergência"
else falha "F3 comparador não viu divergência entre conjuntos diferentes"; fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE FLAGS MARIADB: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE FLAGS MARIADB: REPROVADO"
exit 1
