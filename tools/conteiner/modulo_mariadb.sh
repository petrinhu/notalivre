#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Módulo do cliente MariaDB (Connector/C 3.4.11) nas imagens de build (NL-3.1h, passo 6.2). Uso: modulo_mariadb.sh.
# Baixa o tarball da fonte canônica (archive.mariadb.org), confere o SHA-256 antes de extrair, compila e instala com a
# lista única de opções. A lista é conferida pelo autoteste_flags_mariadb.sh contra o manifesto Flatpak: não mude uma
# sem mudar a outra no mesmo commit. Roda dentro do contêiner (instalar/<distro>.sh); nunca no host.
set -euo pipefail

URL=https://archive.mariadb.org/connector-c-3.4.11/mariadb-connector-c-3.4.11-src.tar.gz
SHA256=521c0712e9291fa96558df9e2ff431376a3a79329f13751896b694cae12765b4
TRABALHO="$(mktemp -d "${TMPDIR:-/var/tmp}/nl-mariadb-c.XXXXXX")"
trap 'rm -rf -- "$TRABALHO"' EXIT

curl -fsSL --proto '=https' --proto-redir '=https' --max-redirs 3 -m 300 -o "$TRABALHO/src.tar.gz" "$URL"
if ! echo "$SHA256  $TRABALHO/src.tar.gz" | sha256sum -c --status; then
    echo "PARADA: SHA-256 do Connector/C diferente do fixado (521c07...65b4)"
    exit 1
fi

mkdir -- "$TRABALHO/fonte"
tar -xzf "$TRABALHO/src.tar.gz" -C "$TRABALHO/fonte" --strip-components=1

# Alvos de plugin desligados na lista: têm de existir na fonte, senão a lista está errada (PARADA).
for alvo in parsec auth_gssapi_client remote_io; do
    if ! grep -rqw -- "TARGET $alvo" "$TRABALHO/fonte/plugins"; then
        echo "PARADA: alvo de plugin $alvo ausente na fonte do Connector/C 3.4.11"
        exit 1
    fi
done

cmake -S "$TRABALHO/fonte" -B "$TRABALHO/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_SSL=OPENSSL \
    -DWITH_EXTERNAL_ZLIB=ON \
    -DWITH_UNIT_TESTS=OFF \
    -DWITH_CURL=OFF \
    -DWITH_DOCS=OFF \
    -DCLIENT_PLUGIN_PARSEC=OFF \
    -DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=OFF \
    -DCLIENT_PLUGIN_REMOTE_IO=OFF \
    > "$TRABALHO/configure.log" 2>&1
echo "rc_configure=$?"

# A linha do TLS tem de dizer OpenSSL, e nenhuma opção desconhecida pode ter saído no configure.
if ! grep -qi 'openssl' "$TRABALHO/configure.log"; then
    echo "PARADA: o configure não mostra o TLS (OpenSSL)"
    exit 1
fi
if grep -q 'Manually-specified variables were not used' "$TRABALHO/configure.log"; then
    echo "PARADA: opção desconhecida no configure do Connector/C"
    grep -A4 'Manually-specified variables were not used' "$TRABALHO/configure.log"
    exit 1
fi

cmake --build "$TRABALHO/build" > "$TRABALHO/build.log" 2>&1
echo "rc_build=$?"
cmake --install "$TRABALHO/build" > "$TRABALHO/install.log" 2>&1
echo "rc_install=$?"
ldconfig
echo "NL-MODULO-MARIADB instalado, Connector/C 3.4.11, SHA-256 conferido"
