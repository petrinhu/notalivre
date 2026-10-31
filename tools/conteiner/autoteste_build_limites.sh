#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da cota de contêiner do build local (NL-2.11; ordem do líder de 30/10/2026: 7 núcleos e 12 GiB para o NotaLivre).
# Roda o executar com um docker de mentira (STUB, nenhum contêiner sobe): todo docker run (portões, diagnóstico e build)
# tem de levar --cpus 7 --memory 12g --memory-swap 12g. Vermelho antes do conserto (L-36). O docker build de imagem não
# entra aqui: não aceita --cpus no mesmo formato.
# Uso: autoteste_build_limites.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-build-limites.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/fixture" "$BASE/tmp"
printf '#!/bin/sh\necho "docker $*" >> "${STUB_LOG:-/dev/null}"\nexit 0\n' > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH"
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Fixture com os caminhos que LISTA_FONTE exige (git archive falha se faltar algum).
F="$BASE/fixture"
for d in src apps tests third_party tools/conteiner tools/tabelas tools/servidor_falso tools/portoes flatpak .github/workflows; do
    mkdir -p "$F/$d"
    printf 'x\n' > "$F/$d/conteudo.txt"
done
printf 'project(x)\n' > "$F/CMakeLists.txt"
printf "name: ci\n" > "$F/.github/workflows/ci.yml"
printf '{}\n' > "$F/CMakePresets.json"
git -C "$F" init -q
git -C "$F" add -A
git -C "$F" -c core.hooksPath=/dev/null -c user.name=autoteste -c user.email=autoteste@invalid commit -q -m "test: fixture" || { echo "ABORTA: commit da fixture"; exit 1; }
SHA="$(git -C "$F" rev-parse HEAD)" || { echo "ABORTA: rev-parse"; exit 1; }

env -u NL_FONTE NL_THREADS=7 NL_MEMORIA=12g NL_FATIA=autoteste-limites NL_REPO="$F" NL_SHA="$SHA" \
    NL_TMP_BASE="$BASE/tmp" STUB_LOG="$BASE/docker.log" NL_LOG="$BASE/build.log" \
    timeout 600 bash "$RAIZ/tools/conteiner/build_local.sh" executar fedora44 debug > "$BASE/saida.txt" 2>&1
rc=$?
total=$(grep -c '^docker run ' "$BASE/docker.log" 2>/dev/null || true)
com=$(grep '^docker run ' "$BASE/docker.log" 2>/dev/null | grep -c -- '--cpus 7 .*--memory 12g .*--memory-swap 12g' || true)
echo "docker_run=$total com_limites=$com rc=$rc"

if [ "${total:-0}" -gt 0 ] && [ "${com:-0}" = "$total" ]; then
    ok "todos os $total docker run do build local levam --cpus 7 --memory 12g --memory-swap 12g"
else
    falha "docker run sem os limites: $(( ${total:-0} - ${com:-0} )) de ${total:-0}"
fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE LIMITES BUILD: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE LIMITES BUILD: REPROVADO"
exit 1
