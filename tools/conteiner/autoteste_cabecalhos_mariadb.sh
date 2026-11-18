#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do pré-cheque do módulo MariaDB nas imagens de build (NL-3.1h, V-b; L-35, L-36). Molde: autoteste_cabecalhos_sodium.sh.
#   M1 cada instalar/ traz o pacote do cliente MariaDB (fedora45, ubuntu26.04, mint22.3, manjaro).
#   M2 precheque_mariadb.sh REPROVA quando <mariadb/mysql.h> falta. O cc de sandbox usa -nostdinc e links de /usr/include
#      sem o diretório mariadb/; a biblioteca de sandbox existe, para o caso isolar só o cabeçalho.
#   M3 controle positivo: um cc de sandbox com um mariadb/mysql.h de uma linha e uma libmariadb.so de sandbox; o precheque aprova.
#      Não depende do Connector/C do host nem do runner.
#   M4 precheque_mariadb.sh é chamado pelo precheque_qt.sh (o CI e o build_local rodam precheque_qt.sh).
# Não sobe contêiner. Uso: autoteste_cabecalhos_mariadb.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-mariadb.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
CHECA="$CONTEINER/precheque_mariadb.sh"

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# M1: pacote do cliente MariaDB em cada um dos quatro scripts instalar/ (piso: 4 analisados).
analisados=0; faltou=0
for par in fedora45:mariadb-connector-c-devel ubuntu26.04:libmariadb-dev mint22.3:libmariadb-dev manjaro:mariadb-libs; do
    arq="${par%%:*}"; pac="${par#*:}"
    analisados=$((analisados + 1))
    grep -qw -- "$pac" "$CONTEINER/instalar/$arq.sh" 2>/dev/null || faltou=$((faltou + 1))
done
echo "M1 cliente mariadb em instalar/: analisados=$analisados falharam=$faltou"
if [ "$analisados" -eq 4 ] && [ "$faltou" -eq 0 ]; then ok "M1 pacote do cliente MariaDB nos quatro instalar/"
else falha "M1 pacote do cliente MariaDB ausente em $faltou de $analisados scripts"; fi

# Sandbox sem o cabeçalho: links de /usr/include exceto mariadb/ e mysql.h; a biblioteca de sandbox existe.
mkdir -p "$BASE/inc" "$BASE/lib"
entradas=()
while IFS= read -r e; do
    case "$e" in mariadb|mysql.h) continue ;; esac
    entradas+=("/usr/include/$e")
done < <(ls -A /usr/include)
ln -s "${entradas[@]}" "$BASE/inc/"
: > "$BASE/lib/libmariadb.so"
printf '#!/bin/sh\ncase "$1" in -print-file-name=*) echo "%s/${1#-print-file-name=}"; exit 0;; esac\nexec cc -nostdinc -isystem "%s" -isystem "%s" "$@"\n' \
    "$BASE/lib" "$(cc -print-file-name=include)" "$BASE/inc" > "$BASE/cc-sem-cabecalho"
chmod +x "$BASE/cc-sem-cabecalho"

# Sandbox com um mariadb/mysql.h de uma linha e a biblioteca de sandbox: controle positivo (M3).
mkdir -p "$BASE/mariadb-ok/mariadb"
printf '#define NOTALIVRE_MARIADB_SANDBOX_AUTOTESTE 1\n' > "$BASE/mariadb-ok/mariadb/mysql.h"
printf '#!/bin/sh\ncase "$1" in -print-file-name=*) echo "%s/${1#-print-file-name=}"; exit 0;; esac\nexec cc -isystem "%s" "$@"\n' \
    "$BASE/lib" "$BASE/mariadb-ok" > "$BASE/cc-com-cabecalho"
chmod +x "$BASE/cc-com-cabecalho"

# M2: sem o cabeçalho, o precheque reprova e cita mariadb/mysql.h.
if [ ! -f "$CHECA" ]; then
    falha "M2 precheque_mariadb.sh ausente"
else
    saida="$(CC="$BASE/cc-sem-cabecalho" bash "$CHECA" 2>&1)"; rc2=$?
    if [ "$rc2" -ne 0 ] && [[ "$saida" == *"mariadb_h=AUSENTE"* ]] && [[ "$saida" == *"mariadb/mysql.h"* ]]; then
        ok "M2 sem cabeçalho: precheque reprova (rc=$rc2) citando mariadb/mysql.h"
    else
        falha "M2 sem cabeçalho o precheque NÃO reprovou (rc=$rc2)"
    fi
fi

# M3: controle positivo com o cc de sandbox que tem o cabeçalho e a biblioteca.
if [ ! -f "$CHECA" ]; then
    falha "M3 precheque_mariadb.sh ausente"
else
    saida="$(CC="$BASE/cc-com-cabecalho" bash "$CHECA" 2>&1)"; rc3=$?
    if [ "$rc3" -eq 0 ] && [[ "$saida" == *"mariadb_h=ok"* ]]; then ok "M3 cc com cabeçalho de sandbox: precheque aprova"
    else falha "M3 cc com cabeçalho reprovado (rc=$rc3): $saida"; fi
fi

# M4: a etapa que o CI e o build_local já rodam chama a checagem.
if grep -q -- 'precheque_mariadb.sh' "$CONTEINER/precheque_qt.sh" 2>/dev/null; then
    ok "M4 precheque_qt.sh chama precheque_mariadb.sh"
else
    falha "M4 precheque_qt.sh não chama precheque_mariadb.sh"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CABECALHOS MARIADB: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CABECALHOS MARIADB: REPROVADO"
exit 1
