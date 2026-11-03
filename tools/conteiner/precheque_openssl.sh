#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Pré-cheque dos cabeçalhos de desenvolvimento do OpenSSL nas imagens de build (NL-2.2; autorização do líder de 28/10/2026).
# Compila um probe com #include <openssl/pkcs12.h> só até a análise sintática (-fsyntax-only: nenhum artefato, nada executa).
# Sem o pacote de cabeçalhos (openssl-devel no fedora44; libssl-dev no ubuntu26.04 e no mint22.3) o cc falha com
# "No such file or directory" e este script sai com 1. A etapa precheque_qt.sh chama este script: o CI e o build_local rodam essa etapa.
# O compilador vem de CC (padrão: cc); o autoteste troca CC por um cc de sandbox sem o cabeçalho.
# Uso: precheque_openssl.sh
set -u

CC_BIN="${CC:-cc}"
probe_dir="$(mktemp -d "${TMPDIR:-/var/tmp}/precheque-openssl.XXXXXX")" || exit 1
trap 'rm -rf -- "$probe_dir"' EXIT
printf '#include <openssl/pkcs12.h>\nint main(void) { return 0; }\n' > "$probe_dir/probe.c"

saida="$("$CC_BIN" -fsyntax-only "$probe_dir/probe.c" 2>&1)"
rc=$?
if [ "$rc" -eq 0 ]; then
    echo "precheque_openssl: cabecalho_pkcs12=ok (cc=$CC_BIN)"
    exit 0
fi
echo "precheque_openssl: cabecalho_pkcs12=AUSENTE (cc=$CC_BIN, rc=$rc)"
printf '%s\n' "$saida"
exit 1
