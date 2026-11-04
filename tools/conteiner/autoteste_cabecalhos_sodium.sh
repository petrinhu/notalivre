#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste dos cabeçalhos de desenvolvimento do libsodium nas imagens de build (NL-3.2a; autorização do líder de 29/10/2026,
# "Autorizo agora (Recomendado)"). TDD do ambiente (L-35, L-36): cada caso foi visto VERMELHO antes da correção.
#   E1 os 4 instalar/ trazem o pacote: fedora44 (libsodium-devel), ubuntu26.04 e mint22.3 (libsodium-dev), manjaro (libsodium).
#   E2 precheque_sodium.sh REPROVA quando <sodium.h> falta. O cc de sandbox usa -nostdinc e links de /usr/include
#      sem sodium.h nem o diretório sodium/; nada é instalado nem alterado.
#   E3 controle positivo: um cc de sandbox com um sodium.h de uma linha em -isystem; o precheque aprova.
#      Não depende do libsodium do host nem do runner.
#   E4 precheque_qt.sh chama precheque_sodium.sh (o CI e o build_local rodam precheque_qt.sh).
# Não sobe contêiner. Uso: autoteste_cabecalhos_sodium.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-sodium.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# E1: pacote de cabeçalhos em cada um dos quatro scripts instalar/ (piso: 4 analisados).
analisados=0; faltou=0
for par in fedora44:libsodium-devel ubuntu26.04:libsodium-dev mint22.3:libsodium-dev manjaro:libsodium; do
    arq="${par%%:*}"; pac="${par#*:}"
    analisados=$((analisados + 1))
    grep -qw -- "$pac" "$CONTEINER/instalar/$arq.sh" 2>/dev/null || faltou=$((faltou + 1))
done
echo "E1 cabecalhos sodium em instalar/: analisados=$analisados falharam=$faltou"
if [ "$analisados" -eq 4 ] && [ "$faltou" -eq 0 ]; then ok "E1 pacote libsodium nos quatro instalar/"
else falha "E1 pacote libsodium ausente em $faltou de $analisados scripts"; fi

# Sandbox sem o cabeçalho: links de /usr/include exceto sodium.h e o diretório sodium/, e um cc que só enxerga esses links.
mkdir -p "$BASE/inc"
entradas=()
while IFS= read -r e; do
    case "$e" in sodium.h|sodium) continue ;; esac
    entradas+=("/usr/include/$e")
done < <(ls -A /usr/include)
ln -s "${entradas[@]}" "$BASE/inc/"
printf '#!/bin/sh\nexec cc -nostdinc -isystem "%s" -isystem "%s" "$@"\n' "$(cc -print-file-name=include)" "$BASE/inc" > "$BASE/cc-sem-sodium"
chmod +x "$BASE/cc-sem-sodium"

# Sandbox com um sodium.h de uma linha, só para o controle positivo (E3).
mkdir -p "$BASE/sodium-ok"
printf '#define NOTALIVRE_SODIUM_SANDBOX_AUTOTESTE 1\n' > "$BASE/sodium-ok/sodium.h"
printf '#!/bin/sh\nexec cc -isystem "%s" "$@"\n' "$BASE/sodium-ok" > "$BASE/cc-com-sodium"
chmod +x "$BASE/cc-com-sodium"

CHECA="$CONTEINER/precheque_sodium.sh"

# E2: sem o cabeçalho, o precheque reprova e cita sodium.h.
if [ ! -f "$CHECA" ]; then
    falha "E2 precheque_sodium.sh ausente"
else
    saida="$(CC="$BASE/cc-sem-sodium" bash "$CHECA" 2>&1)"; rc2=$?
    if [ "$rc2" -ne 0 ] && [[ "$saida" == *"sodium_h=AUSENTE"* ]] && [[ "$saida" == *"sodium.h"* ]]; then
        ok "E2 sem cabeçalho: precheque reprova (rc=$rc2) citando sodium.h"
    else
        falha "E2 sem cabeçalho o precheque NÃO reprovou (rc=$rc2)"
    fi
fi

# E3: controle positivo com o cc de sandbox que tem o sodium.h de uma linha.
if [ ! -f "$CHECA" ]; then
    falha "E3 precheque_sodium.sh ausente"
else
    saida="$(CC="$BASE/cc-com-sodium" bash "$CHECA" 2>&1)"; rc3=$?
    if [ "$rc3" -eq 0 ] && [[ "$saida" == *"sodium_h=ok"* ]]; then ok "E3 cc com sodium.h de sandbox: precheque aprova"
    else falha "E3 cc com sodium.h reprovado (rc=$rc3): $saida"; fi
fi

# E4: a etapa que o CI e o build_local já rodam chama a checagem.
if grep -q -- 'precheque_sodium.sh' "$CONTEINER/precheque_qt.sh" 2>/dev/null; then
    ok "E4 precheque_qt.sh chama precheque_sodium.sh"
else
    falha "E4 precheque_qt.sh não chama precheque_sodium.sh"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CABECALHOS SODIUM: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CABECALHOS SODIUM: REPROVADO"
exit 1
