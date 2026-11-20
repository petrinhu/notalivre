#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Pré-cheque do cliente MariaDB (Connector/C) nas imagens de build (NL-3.1h, V-b e v3.17; decisão do líder de 04/11/2026,
# "Biblioteca direto (Recomendado)"). Cabeçalho e biblioteca vêm do MESMO prefixo: NL_MARIADB_PREFIX, ou o da distro NL_DISTRO
# em tools/conteiner/prefixo_mariadb.txt. O cabeçalho é <prefixo>/include/mariadb/mysql.h (layout DEFAULT e Debian) ou
# <prefixo>/include/mysql/mysql.h (Arch e RPM), e o probe o inclui como <mysql.h>, com -I, como manda a documentação do
# Connector/C. A biblioteca é libmariadb.so em <prefixo>/lib/mariadb, <prefixo>/lib64 ou <prefixo>/lib. Sem prefixo, sem
# cabeçalho ou sem biblioteca: rc 1. Compila só até a análise sintática (nada executa) e não instala nada.
# O compilador vem de CC (padrão: cc). Uso: precheque_mariadb.sh
set -u

CC_BIN="${CC:-cc}"
prefixo="${NL_MARIADB_PREFIX:-}"
if [ -z "$prefixo" ]; then
    . "$(dirname "${BASH_SOURCE[0]}")/prefixo_mariadb.sh"
    prefixo="$(nl_prefixo_mariadb "${NL_DISTRO:-}")" || prefixo=""
fi
if [ -z "$prefixo" ]; then
    echo "precheque_mariadb: prefixo=AUSENTE (sem NL_MARIADB_PREFIX, e NL_DISTRO '${NL_DISTRO:-}' sem linha em prefixo_mariadb.txt)"
    exit 1
fi
inc=""
for d in "$prefixo/include/mariadb" "$prefixo/include/mysql"; do
    if [ -f "$d/mysql.h" ]; then inc="$d"; break; fi
done
lib=""
for d in "$prefixo/lib/mariadb" "$prefixo/lib64" "$prefixo/lib"; do
    if [ -f "$d/libmariadb.so" ]; then lib="$d/libmariadb.so"; break; fi
done
rc_cabecalho=1
saida=""
if [ -n "$inc" ]; then
    probe_dir="$(mktemp -d "${TMPDIR:-/var/tmp}/precheque-mariadb.XXXXXX")" || exit 1
    trap 'rm -rf -- "$probe_dir"' EXIT
    printf '#include <mysql.h>\nint main(void) { return 0; }\n' > "$probe_dir/probe.c"
    saida="$("$CC_BIN" -fsyntax-only -I"$inc" "$probe_dir/probe.c" 2>&1)"
    rc_cabecalho=$?
fi
if [ "$rc_cabecalho" -eq 0 ] && [ -n "$lib" ]; then
    echo "precheque_mariadb: mariadb_h=ok libmariadb=ok (prefixo=$prefixo, cabecalho=$inc/mysql.h, biblioteca=$lib, cc=$CC_BIN)"
    exit 0
fi
if [ "$rc_cabecalho" -ne 0 ]; then
    echo "precheque_mariadb: mariadb_h=AUSENTE (prefixo=$prefixo; procurado em include/mariadb e include/mysql; rc=$rc_cabecalho)"
    [ -z "$saida" ] || printf '%s\n' "$saida"
fi
if [ -z "$lib" ]; then
    echo "precheque_mariadb: libmariadb=AUSENTE (prefixo=$prefixo; procurado em lib/mariadb, lib64 e lib)"
fi
exit 1
