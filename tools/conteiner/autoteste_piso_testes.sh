#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da trava de piso de testes do job nativo (rodada 5, CI da onda-2, run 40804430962): sem prova de vermelho,
# a trava não conta (L-36). Cada caso monta um CMakeCache.txt e chama avaliar() com uma contagem conhecida.
# Usa uma pasta fixa em NL_TMP_BASE e sobrescreve os arquivos: nada é apagado (L-53).
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TRAVA="$RAIZ/tools/conteiner/piso_testes.sh"
BASE="${NL_TMP_BASE:-/var/tmp}/notalivre-piso-autoteste"
FALHAS=0

mkdir -p "$BASE" || exit 1
CACHE_OFF="$BASE/cache-off.txt"
CACHE_ON="$BASE/cache-on.txt"
CACHE_SEM_LINHA="$BASE/cache-sem-linha.txt"
printf 'NL_XMLSEC1_SF:FILEPATH=/usr/bin/xmlsec1\nNL_FLATPAK_BUILD:BOOL=OFF\n' > "$CACHE_OFF"
printf 'NL_FLATPAK_BUILD:BOOL=ON\n' > "$CACHE_ON"
printf 'NL_XMLSEC1_SF:FILEPATH=/usr/bin/xmlsec1\n' > "$CACHE_SEM_LINHA"

# veredito NOME RC_ESPERADO RC_OBTIDO SAIDA: uma linha por caso; falha se o rc não bater.
veredito() {
    if [ "$3" -eq "$2" ]; then
        echo "[VERDE] $1: rc=$3 (esperado $2) $4"
    else
        echo "[VERMELHO] $1: rc=$3 (esperado $2) $4"
        FALHAS=$((FALHAS + 1))
    fi
}

if [ ! -f "$TRAVA" ]; then
    echo "[VERMELHO] trava ausente: $TRAVA"
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
# shellcheck source=/dev/null
. "$TRAVA"

saida=$(avaliar 95 109 "$CACHE_OFF"); rc=$?
veredito "abaixo do piso reprova (95 < 109)" 1 "$rc" "$saida"
saida=$(avaliar 109 109 "$CACHE_OFF"); rc=$?
veredito "no piso passa (109 = 109)" 0 "$rc" "$saida"
saida=$(avaliar 110 109 "$CACHE_OFF"); rc=$?
veredito "acima do piso passa (110 > 109)" 0 "$rc" "$saida"
saida=$(avaliar 110 109 "$CACHE_ON"); rc=$?
veredito "NL_FLATPAK_BUILD ON fora do Flatpak reprova mesmo com contagem boa" 1 "$rc" "$saida"
saida=$(avaliar 109 109 "$BASE/cache-inexistente.txt"); rc=$?
veredito "cache ausente reprova" 1 "$rc" "$saida"
saida=$(avaliar 109 109 "$CACHE_SEM_LINHA"); rc=$?
veredito "cache sem a linha NL_FLATPAK_BUILD reprova" 1 "$rc" "$saida"
saida=$(avaliar "" 109 "$CACHE_OFF"); rc=$?
veredito "contagem ilegível reprova" 1 "$rc" "$saida"

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -ne 0 ]; then
    echo "AUTOTESTE: FALHOU"
    exit 1
fi
echo "AUTOTESTE: todos os controles conferem"
