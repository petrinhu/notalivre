#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste dos cabeçalhos de desenvolvimento do OpenSSL nas imagens de build (NL-2.2; autorização do líder de 28/10/2026,
# "Autorizo nas 3 imagens (Recomendado)"). TDD do ambiente (L-35, L-36): cada caso foi visto VERMELHO antes da correção.
#   E1 fedora44, ubuntu26.04 e mint22.3 instalam o pacote de cabeçalhos (openssl-devel e libssl-dev).
#   E2 precheque_openssl.sh REPROVA quando <openssl/pkcs12.h> falta. O cc de sandbox usa -nostdinc e uma cópia de
#      /usr/include sem o diretório openssl; nada é instalado nem alterado no host.
#   E3 controle positivo: com o cc normal (cabeçalhos presentes) o precheque aprova.
#   E4 precheque_qt.sh chama precheque_openssl.sh (o CI e o build_local rodam precheque_qt.sh).
# Não sobe contêiner. Uso: autoteste_cabecalhos_openssl.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-openssl.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# E1: pacote de cabeçalhos em cada um dos três scripts (piso: 3 analisados).
analisados=0; faltou=0
for par in fedora44:openssl-devel ubuntu26.04:libssl-dev mint22.3:libssl-dev; do
    arq="${par%%:*}"; pac="${par#*:}"
    analisados=$((analisados + 1))
    grep -q -- "$pac" "$CONTEINER/instalar/$arq.sh" 2>/dev/null || faltou=$((faltou + 1))
done
echo "E1 cabecalhos em instalar/: analisados=$analisados falharam=$faltou"
if [ "$analisados" -eq 3 ] && [ "$faltou" -eq 0 ]; then ok "E1 pacote de cabeçalhos nos três instalar/"
else falha "E1 pacote de cabeçalhos ausente em $faltou de $analisados scripts"; fi

# Sandbox sem o cabeçalho: links de /usr/include exceto openssl, e um cc que só enxerga esses links.
mkdir -p "$BASE/inc"
entradas=()
while IFS= read -r e; do
    [ "$e" = openssl ] || entradas+=("/usr/include/$e")
done < <(ls -A /usr/include)
ln -s "${entradas[@]}" "$BASE/inc/"
printf '#!/bin/sh\nexec cc -nostdinc -isystem "%s" -isystem "%s" "$@"\n' "$(cc -print-file-name=include)" "$BASE/inc" > "$BASE/cc-sem-openssl"
chmod +x "$BASE/cc-sem-openssl"
CHECA="$CONTEINER/precheque_openssl.sh"

# E2: sem o cabeçalho, o precheque reprova e cita o arquivo que falta.
if [ ! -f "$CHECA" ]; then
    falha "E2 precheque_openssl.sh ausente"
else
    saida="$(CC="$BASE/cc-sem-openssl" bash "$CHECA" 2>&1)"; rc2=$?
    if [ "$rc2" -ne 0 ] && [[ "$saida" == *"cabecalho_pkcs12=AUSENTE"* ]] && [[ "$saida" == *"openssl/pkcs12.h"* ]]; then
        ok "E2 sem cabeçalho: precheque reprova (rc=$rc2) citando openssl/pkcs12.h"
    else
        falha "E2 sem cabeçalho o precheque NÃO reprovou (rc=$rc2)"
    fi
fi

# E3: controle positivo com o cc normal.
if [ ! -f "$CHECA" ]; then
    falha "E3 precheque_openssl.sh ausente"
else
    saida="$(bash "$CHECA" 2>&1)"; rc3=$?
    if [ "$rc3" -eq 0 ] && [[ "$saida" == *"cabecalho_pkcs12=ok"* ]]; then ok "E3 cc normal com cabeçalho: precheque aprova"
    else falha "E3 cc normal reprovado (rc=$rc3): $saida"; fi
fi

# E4: a etapa que o CI e o build_local já rodam chama a checagem.
if grep -q -- 'precheque_openssl.sh' "$CONTEINER/precheque_qt.sh" 2>/dev/null; then
    ok "E4 precheque_qt.sh chama precheque_openssl.sh"
else
    falha "E4 precheque_qt.sh não chama precheque_openssl.sh"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CABECALHOS OPENSSL: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CABECALHOS OPENSSL: REPROVADO"
exit 1
