#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da cota de contêiner do build local (NL-2.11; ordem do líder de 30/10/2026: 7 núcleos e 12 GiB para o NotaLivre).
# Roda o build_local com um docker de mentira (STUB, nenhum contêiner sobe) em dois caminhos: "executar" (portões,
# diagnóstico, configure, build e ctest) e "qt" (instalação do Qt, com o pré-cheque "test -f" devolvendo ausente).
# Todo docker run tem de levar --cpus 7 --memory 12g --memory-swap 12g, com as cotas PADRÃO (sem NL_THREADS nem
# NL_MEMORIA exportadas: o autoteste mede o default, não o valor que ele mesmo passa). Vermelho antes do conserto (L-36).
# O docker build de imagem não entra aqui: não aceita --cpus no mesmo formato.
# Uso: autoteste_build_limites.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-build-limites.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/fixture" "$BASE/tmp"
cat > "$BASE/bin/docker" <<'STUB'
#!/bin/sh
echo "docker $*" >> "${STUB_LOG:-/dev/null}"
case "$*" in
    *"test -f"*) exit "${STUB_TESTF_RC:-0}" ;;
esac
exit 0
STUB
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

# Conta e confere: todo docker run do log tem de levar as três cotas. Zero chamadas é falha (varredura vazia, L-36).
conferir() {
    local log="$1" nome="$2" total com
    total=$(grep -c '^docker run ' "$log" 2>/dev/null || true)
    com=$(grep '^docker run ' "$log" 2>/dev/null | grep -c -- '--cpus 7 .*--memory 12g .*--memory-swap 12g' || true)
    echo "$nome: docker_run=$total com_limites=$com"
    if [ "${total:-0}" -gt 0 ] && [ "${com:-0}" = "$total" ]; then
        ok "$nome: todos os $total docker run levam --cpus 7 --memory 12g --memory-swap 12g"
    else
        falha "$nome: docker run sem os limites: $(( ${total:-0} - ${com:-0} )) de ${total:-0}"
    fi
}

# Caminho executar (portões, diagnóstico, configure, build e ctest), com as cotas padrão.
env -u NL_FONTE -u NL_THREADS -u NL_MEMORIA NL_FATIA=autoteste-limites NL_REPO="$F" NL_SHA="$SHA" \
    NL_TMP_BASE="$BASE/tmp" STUB_LOG="$BASE/docker.log" NL_LOG="$BASE/build.log" \
    timeout 600 bash "$RAIZ/tools/conteiner/build_local.sh" executar fedora44 debug > "$BASE/saida.txt" 2>&1
rc=$?
echo "executar: rc=$rc"
conferir "$BASE/docker.log" executar

# Caminho qt (instalação do Qt, DA-2.x): o pré-cheque "test -f" devolve ausente, então as três chamadas do aqt rodam.
env -u NL_FONTE -u NL_THREADS -u NL_MEMORIA NL_FATIA=autoteste-limites NL_REPO="$F" NL_SHA="$SHA" \
    NL_TMP_BASE="$BASE/tmp" STUB_LOG="$BASE/docker-qt.log" STUB_TESTF_RC=1 \
    timeout 600 bash "$RAIZ/tools/conteiner/build_local.sh" qt > "$BASE/saida-qt.txt" 2>&1
rc_qt=$?
echo "qt: rc=$rc_qt"
conferir "$BASE/docker-qt.log" qt

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE LIMITES BUILD: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE LIMITES BUILD: REPROVADO"
exit 1
