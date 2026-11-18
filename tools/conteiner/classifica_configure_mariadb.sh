#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Classificador do Connector/C (NL-3.1h, passo 6.2, v3.10). Uso:
#   classifica_configure_mariadb.sh configure <configure.log>
#   classifica_configure_mariadb.sh plugins <install_manifest.txt>
# configure: aceita o aviso "Manually-specified variables were not used" só para CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT, e só
#   sem "Found GSSAPI" (sem GSSAPI, o plugin não se registra por desenho). Qualquer outro nome não usado é rc 1.
# plugins: rc 1 se o install_manifest tem parsec.so, auth_gssapi_client.so ou remote_io.so, ou se não tem caching_sha2_password.so.
# Sai 2 no uso errado. Imprime sempre a linha NL-CONFIGURE ou NL-PLUGINS. Não sobe contêiner.
set -u

PROIBIDOS_RE='/(parsec|auth_gssapi_client|remote_io)\.so$'
CACHING_RE='/caching_sha2_password\.so$'

# Nomes do bloco "Manually-specified variables were not used", um por linha, sem o prefixo de saída do buildkit.
nomes_nao_usados() {
    sed -E 's/^#[0-9]+ [0-9.]+ //' "$1" | awk '
        /Manually-specified variables were not used/ { ativo = 1; nomes = 0; next }
        ativo && /^[ \t]+[A-Z][A-Z0-9_]+[ \t]*$/ { gsub(/[ \t]/, ""); print; nomes = 1; next }
        ativo && nomes && /^[ \t]*$/ { ativo = 0 }'
}

configure() {
    local log="$1" nomes gssapi="ausente" rc=0 toleradas="" todos="" n
    if [ ! -s "$log" ]; then
        echo "NL-CONFIGURE nao_usadas=ausente toleradas=nenhuma gssapi=ausente"
        echo "PARADA: log do configure ausente ou vazio"
        return 1
    fi
    if ! grep -q 'Configuring done' "$log"; then
        echo "NL-CONFIGURE nao_usadas=ausente toleradas=nenhuma gssapi=ausente"
        echo "PARADA: o configure não concluiu (sem Configuring done)"
        return 1
    fi
    grep -q 'Found GSSAPI' "$log" && gssapi=achado
    nomes="$(nomes_nao_usados "$log")"
    for n in $nomes; do
        todos="$todos $n"
        if [ "$n" = CLIENT_PLUGIN_AUTH_GSSAPI_CLIENT ] && [ "$gssapi" = ausente ]; then
            toleradas="$toleradas $n"
        else
            echo "PARADA: opção não usada fora da tolerância: $n"
            rc=1
        fi
    done
    echo "NL-CONFIGURE nao_usadas=${todos:- nenhuma} toleradas=${toleradas:- nenhuma} gssapi=$gssapi"
    return "$rc"
}

plugins() {
    local manifesto="$1" proibidos caching
    if [ ! -s "$manifesto" ]; then
        echo "NL-PLUGINS proibidos=ausente caching_sha2=ausente"
        echo "PARADA: install_manifest ausente ou vazio"
        return 1
    fi
    proibidos="$(grep -cE -- "$PROIBIDOS_RE" "$manifesto" || true)"
    caching="$(grep -cE -- "$CACHING_RE" "$manifesto" || true)"
    echo "NL-PLUGINS proibidos=$proibidos caching_sha2=$caching"
    if [ "$proibidos" -ne 0 ]; then
        echo "PARADA: plugin proibido instalado"; return 1
    fi
    if [ "$caching" -lt 1 ]; then
        echo "PARADA: caching_sha2_password.so ausente (controle positivo)"; return 1
    fi
    return 0
}

case "${1:-}" in
    configure) [ $# -eq 2 ] || { echo "uso: classifica_configure_mariadb.sh configure <configure.log>" >&2; exit 2; }; configure "$2" ;;
    plugins) [ $# -eq 2 ] || { echo "uso: classifica_configure_mariadb.sh plugins <install_manifest.txt>" >&2; exit 2; }; plugins "$2" ;;
    *) echo "uso: classifica_configure_mariadb.sh configure|plugins <arquivo>" >&2; exit 2 ;;
esac
