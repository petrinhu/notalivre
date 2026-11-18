#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da lista única de opções do Connector/C (NL-3.1h, V-c; L-35, L-36, L-17). A lista mora em modulo_mariadb.sh e
# no manifesto Flatpak, e este autoteste amarra os dois: o conjunto de opções -D tem de ser igual nos dois, e tem de trazer
# as 9 opções fixadas no passo 6.2 da ordem. Vermelho antes do módulo: modulo_mariadb.sh e o manifesto ainda não têm a lista.
#   F1 modulo_mariadb.sh traz as 9 opções fixadas.
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

# Opções fixadas no passo 6.2 da ordem v3.1 (lista única; mudança aqui exige mudança no passo 6.2 no mesmo commit).
ESPERADAS="-DCMAKE_BUILD_TYPE=Release
-DWITH_SSL=OPENSSL
-DWITH_EXTERNAL_ZLIB=ON
-DWITH_UNIT_TESTS=OFF
-DWITH_CURL=OFF
-DWITH_DOCS=OFF
-DCLIENT_PLUGIN_PARSEC=OFF
-DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=OFF
-DCLIENT_PLUGIN_REMOTE_IO=OFF"

# Conjunto ordenado das opções -D de um arquivo; arquivo ausente sai com rc 1 (nunca conjunto vazio silencioso).
opcoes_de() {
    [ -f "$1" ] || return 1
    grep -oE -- '-D[A-Z_]+=[A-Za-z0-9_]+' "$1" | sort -u
}

# F1: o script traz as 9 opções esperadas, todas, sem sobra de nome diferente.
if conjunto_modulo="$(opcoes_de "$MODULO")"; then
    faltam="$(printf '%s\n' "$ESPERADAS" | sort -u | comm -23 - <(printf '%s\n' "$conjunto_modulo"))"
    if [ -z "$faltam" ]; then ok "F1 modulo_mariadb.sh traz as 9 opções fixadas"
    else falha "F1 modulo_mariadb.sh sem opções fixadas: $(printf '%s' "$faltam" | tr '\n' ' ')"; fi
else
    falha "F1 modulo_mariadb.sh ausente: sem lista de opções do Connector/C"
    conjunto_modulo=""
fi

# F2: manifesto e script com o mesmo conjunto de opções.
if conjunto_manifesto="$(opcoes_de "$MANIFESTO")"; then
    if [ -n "$conjunto_modulo" ] && [ "$conjunto_manifesto" = "$conjunto_modulo" ]; then
        ok "F2 manifesto e modulo_mariadb.sh têm o mesmo conjunto de opções"
    else
        falha "F2 manifesto e modulo_mariadb.sh divergem, ou o script não tem lista"
    fi
else
    falha "F2 manifesto Flatpak ausente ou sem o módulo mariadb-connector-c"
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
