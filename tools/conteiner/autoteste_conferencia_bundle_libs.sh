#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da conferência do bundle Flatpak (NL-3.2a, parte 2). TDD do portão (L-35, L-36): cada caso foi visto
# VERMELHO antes da implementação. Árvores de teste são sintéticas, criadas em TMPDIR e apagadas no fim.
#   R1 sem libsodium.so* em files/lib: a conferência reprova e cita a biblioteca ausente.
#   R2 sem a pasta de licença: a conferência reprova e cita a licença ausente.
#   R3 árvore vazia: a varredura acha zero arquivos; a conferência reprova com analisados=0 (zero não é bundle limpo).
#   G1 controle positivo: com a biblioteca e a licença, a conferência aprova e imprime analisados maior que zero.
# Não sobe contêiner e não executa nada do bundle. Uso: autoteste_conferencia_bundle_libs.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONFERE="$RAIZ/tools/conteiner/conferencia_bundle_libs.sh"
ID="io.github.petrinhu.notalivre"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-bundle.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Fixtures: a biblioteca e a licença são arquivos vazios; a conferência olha nomes e presença, nunca o conteúdo.
mkdir -p "$BASE/sem-lib/files/share/licenses/$ID"
echo "licenca de teste" > "$BASE/sem-lib/files/share/licenses/$ID/LICENSE"

mkdir -p "$BASE/sem-licenca/files/lib"
: > "$BASE/sem-licenca/files/lib/libsodium.so.26"

mkdir -p "$BASE/vazia"

mkdir -p "$BASE/completa/files/lib" "$BASE/completa/files/share/licenses/$ID"
: > "$BASE/completa/files/lib/libsodium.so.26"
echo "licenca de teste" > "$BASE/completa/files/share/licenses/$ID/LICENSE"

if [ ! -f "$CONFERE" ]; then
    falha "R1 conferencia_bundle_libs.sh ausente"
    falha "R2 conferencia_bundle_libs.sh ausente"
    falha "R3 conferencia_bundle_libs.sh ausente"
    falha "G1 conferencia_bundle_libs.sh ausente"
else
    # R1: sem a biblioteca, rc diferente de zero e a biblioteca citada.
    saida="$(bash "$CONFERE" "$BASE/sem-lib" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"analisados="* ]] && [[ "$saida" == *"libsodium"* ]] && [[ "$saida" == *"AUSENTE"* ]]; then
        ok "R1 sem libsodium: reprova (rc=$rc) e cita a biblioteca"
    else
        falha "R1 sem libsodium NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # R2: sem a pasta de licença, rc diferente de zero e a licença citada.
    saida="$(bash "$CONFERE" "$BASE/sem-licenca" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"licenses"* ]] && [[ "$saida" == *"AUSENTE"* ]]; then
        ok "R2 sem licença: reprova (rc=$rc) e cita share/licenses"
    else
        falha "R2 sem licença NÃO reprovou pelo motivo certo (rc=$rc): $saida"
    fi

    # R3: varredura vazia reprova, com analisados=0 impresso.
    saida="$(bash "$CONFERE" "$BASE/vazia" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"analisados=0"* ]]; then
        ok "R3 árvore vazia: reprova (rc=$rc) com analisados=0"
    else
        falha "R3 árvore vazia NÃO reprovou com analisados=0 (rc=$rc): $saida"
    fi

    # G1: controle positivo, rc zero e analisados maior que zero.
    saida="$(bash "$CONFERE" "$BASE/completa" 2>&1)"; rc=$?
    if [ "$rc" -eq 0 ] && [[ "$saida" == *"falharam=0"* ]] && [[ "$saida" != *"analisados=0"* ]]; then
        ok "G1 bundle completo: aprova (rc=$rc)"
    else
        falha "G1 bundle completo reprovou (rc=$rc): $saida"
    fi
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE CONFERENCIA BUNDLE LIBS: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE CONFERENCIA BUNDLE LIBS: REPROVADO"
exit 1
