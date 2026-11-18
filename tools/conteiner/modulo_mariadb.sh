#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Módulo do cliente MariaDB (Connector/C 3.4.11) nas imagens de build (NL-3.1h, passo 6.2, emendado pela v3.10). Uso:
# modulo_mariadb.sh. Baixa o tarball da fonte canônica (archive.mariadb.org), confere o SHA-256 antes de extrair, compila
# com a lista única de opções, confere o configure e confere o resultado instalado (classifica_configure_mariadb.sh).
# A lista é conferida pelo autoteste_flags_mariadb.sh contra o manifesto Flatpak: não mude uma sem mudar a outra no mesmo
# commit. Roda dentro do contêiner (instalar/<distro>.sh); nunca no host.
set -euo pipefail

URL=https://archive.mariadb.org/connector-c-3.4.11/mariadb-connector-c-3.4.11-src.tar.gz
SHA256=521c0712e9291fa96558df9e2ff431376a3a79329f13751896b694cae12765b4
DIR_MODULO="$(cd "$(dirname "$0")" && pwd)"
CLASSIFICA="$DIR_MODULO/classifica_configure_mariadb.sh"
TRABALHO="$(mktemp -d "${TMPDIR:-/var/tmp}/nl-mariadb-c.XXXXXX")"
trap 'rm -rf -- "$TRABALHO"' EXIT

# Lista única de opções do configure (oito). Mudança aqui exige mudança no manifesto no mesmo commit.
OPCOES=(
    -DCMAKE_BUILD_TYPE=Release
    -DWITH_SSL=OPENSSL
    -DWITH_EXTERNAL_ZLIB=ON
    -DWITH_UNIT_TESTS=OFF
    -DWITH_CURL=OFF
    -DWITH_DOCS=OFF
    -DCLIENT_PLUGIN_PARSEC=OFF
    -DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=OFF
)

curl -fsSL --proto '=https' --proto-redir '=https' --max-redirs 3 -m 300 -o "$TRABALHO/src.tar.gz" "$URL"
if ! echo "$SHA256  $TRABALHO/src.tar.gz" | sha256sum -c --status; then
    echo "PARADA: SHA-256 do Connector/C diferente do fixado (521c07...65b4)"
    exit 1
fi

mkdir -- "$TRABALHO/fonte"
tar -xzf "$TRABALHO/src.tar.gz" -C "$TRABALHO/fonte" --strip-components=1

# Cada opção CLIENT_PLUGIN_<X>=OFF da lista tem de ter o alvo <x> e a variável no cmake da fonte; senão a lista está errada.
for opt in "${OPCOES[@]}"; do
    case "$opt" in
        -DCLIENT_PLUGIN_*=OFF)
            x="${opt#-DCLIENT_PLUGIN_}"; x="${x%=OFF}"
            alvo="$(printf '%s' "$x" | tr 'A-Z' 'a-z')"
            if ! grep -rqw -- "TARGET $alvo" "$TRABALHO/fonte/plugins"; then
                echo "PARADA: alvo de plugin $alvo ($opt) ausente na fonte do Connector/C 3.4.11"
                exit 1
            fi
            if ! grep -qF 'CLIENT_PLUGIN_' "$TRABALHO/fonte/cmake/plugins.cmake"; then
                echo "PARADA: cmake/plugins.cmake sem CLIENT_PLUGIN_ (opção $opt sem efeito)"
                exit 1
            fi
            ;;
    esac
done

if ! cmake -S "$TRABALHO/fonte" -B "$TRABALHO/build" -G Ninja "${OPCOES[@]}" > "$TRABALHO/configure.log" 2>&1; then
    echo "rc_configure=falhou"
    tail -n 40 "$TRABALHO/configure.log"
    exit 1
fi
echo "rc_configure=0"

# O configure só pode trazer o aviso de opção não usada da tolerância (classifica_configure_mariadb.sh configure).
if ! bash "$CLASSIFICA" configure "$TRABALHO/configure.log"; then
    tail -n 40 "$TRABALHO/configure.log"
    exit 1
fi
if ! grep -qi 'openssl' "$TRABALHO/configure.log"; then
    echo "PARADA: o configure não mostra o TLS (OpenSSL)"
    exit 1
fi

cmake --build "$TRABALHO/build" -j 2 > "$TRABALHO/build.log" 2>&1
echo "rc_build=$?"
cmake --install "$TRABALHO/build" > "$TRABALHO/install.log" 2>&1
echo "rc_install=$?"
ldconfig

# O efeito das opções aparece no que foi instalado: nenhum plugin proibido, e o caching_sha2_password presente.
if ! bash "$CLASSIFICA" plugins "$TRABALHO/build/install_manifest.txt"; then
    exit 1
fi
echo "NL-MODULO-MARIADB instalado, Connector/C 3.4.11, SHA-256 conferido"
