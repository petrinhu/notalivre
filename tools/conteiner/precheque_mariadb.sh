#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Pré-cheque do módulo do cliente MariaDB (Connector/C) nas imagens de build (NL-3.1h, V-b; decisão do líder de 04/11/2026,
# "Biblioteca direto (Recomendado)"). Compila um probe com #include <mariadb/mysql.h> só até a análise sintática
# (-fsyntax-only: nenhum artefato, nada executa) e confere se o cc acha a libmariadb.so por -print-file-name.
# Sem o cabeçalho ou a biblioteca do módulo, o script sai com 1. Só lê: não instala nada.
# O compilador vem de CC (padrão: cc); o autoteste troca CC por um cc de sandbox.
# Uso: precheque_mariadb.sh
set -u

CC_BIN="${CC:-cc}"
probe_dir="$(mktemp -d "${TMPDIR:-/var/tmp}/precheque-mariadb.XXXXXX")" || exit 1
trap 'rm -rf -- "$probe_dir"' EXIT
printf '#include <mariadb/mysql.h>\nint main(void) { return 0; }\n' > "$probe_dir/probe.c"

saida="$("$CC_BIN" -fsyntax-only "$probe_dir/probe.c" 2>&1)"
rc_cabecalho=$?
lib="$("$CC_BIN" -print-file-name=libmariadb.so 2>/dev/null)"

if [ "$rc_cabecalho" -eq 0 ] && [ -f "$lib" ]; then
    echo "precheque_mariadb: mariadb_h=ok libmariadb=ok (cc=$CC_BIN)"
    exit 0
fi
if [ "$rc_cabecalho" -ne 0 ]; then
    echo "precheque_mariadb: mariadb_h=AUSENTE (cc=$CC_BIN, rc=$rc_cabecalho)"
    printf '%s\n' "$saida"
fi
if [ ! -f "$lib" ]; then
    echo "precheque_mariadb: libmariadb=AUSENTE (cc=$CC_BIN, lib=$lib)"
fi
exit 1
