#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste dos limites de contêiner dos portões estáticos (NL-2.11; cota dividida com outra sessão: 7 núcleos e 12 GiB).
# Todo docker run do portoes_estaticos.sh leva --cpus 7 --memory 12g --memory-swap 12g. O docker é um STUB que só
# registra as chamadas. Vermelho antes do conserto (L-36): as chamadas não levam os limites.
# Uso: autoteste_portoes_limites.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-limites.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/fixture"
printf '#!/bin/sh\necho "docker $*" >> "%s/docker.log"\nexit 0\n' "$BASE" > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH"
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Repositório de brincadeira com um commit (o portão clona um SHA dele).
git -C "$BASE/fixture" init -q
printf 'x\n' > "$BASE/fixture/arquivo.txt"
git -C "$BASE/fixture" add -A
git -C "$BASE/fixture" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture" || { echo "ABORTA: commit da fixture"; exit 1; }
SHA="$(git -C "$BASE/fixture" rev-parse HEAD)" || { echo "ABORTA: rev-parse"; exit 1; }

timeout 300 bash "$RAIZ/tools/conteiner/portoes_estaticos.sh" "$BASE/fixture" "$SHA" "$BASE/portoes" > "$BASE/saida.txt" 2>&1
rc=$?
chamadas=$(grep -c '^docker run ' "$BASE/docker.log" 2>/dev/null || true)
com_limites=$(grep '^docker run ' "$BASE/docker.log" 2>/dev/null | grep -c -- '--cpus 7 .*--memory 12g .*--memory-swap 12g' || true)
echo "chamadas_docker=$chamadas com_limites=$com_limites rc=$rc"

if [ "${chamadas:-0}" -gt 0 ] && [ "${com_limites:-0}" = "$chamadas" ]; then
    ok "todas as $chamadas chamadas de docker run levam --cpus 7 --memory 12g --memory-swap 12g"
else
    falha "chamadas sem os limites: $(( ${chamadas:-0} - ${com_limites:-0} )) de ${chamadas:-0}"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE LIMITES PORTOES: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE LIMITES PORTOES: REPROVADO"
exit 1
