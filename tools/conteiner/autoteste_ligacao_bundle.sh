#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da ligação das conferências do bundle (NL-3.2a, revisão A1 e A2). TDD de ligação (L-36): cada caso foi visto
# VERMELHO antes da ligação. Lê arquivos de texto; não sobe contêiner e não executa nada do projeto.
#   L1 o job flatpak do ci.yml declara build-dir: flatpak_app no passo do builder (diretório que a action usa por padrão).
#   L2 o job flatpak do ci.yml roda conferencia_bundle_libs.sh flatpak_app.
#   L3 o job flatpak do ci.yml roda conferencia_repo_flatpak.sh repo (repositório OSTree gerado pelo builder).
#   L4 o job_flatpak.sh do espelho roda conferencia_repo_flatpak.sh, para o espelho espelhar o CI.
# Uso: autoteste_ligacao_bundle.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CI="$RAIZ/.github/workflows/ci.yml"
ESPELHO="$RAIZ/tools/espelho/job_flatpak.sh"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-ligacao.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

# Só o trecho do job flatpak: do cabeçalho "  flatpak:" até o início do job seguinte, "  smoke-host:".
JOB="$BASE/job-flatpak.yml"
awk '/^  flatpak:/ { dentro = 1 } /^  smoke-host:/ { dentro = 0 } dentro' "$CI" > "$JOB"
if [ ! -s "$JOB" ]; then
    echo "JOB flatpak não encontrado em $CI: a varredura saiu vazia"
    exit 1
fi

if /usr/bin/grep -q -- 'build-dir: flatpak_app' "$JOB"; then ok "L1 build-dir: flatpak_app no passo do builder"
else falha "L1 o job flatpak não declara build-dir: flatpak_app"; fi

if /usr/bin/grep -q -- 'conferencia_bundle_libs.sh flatpak_app' "$JOB"; then ok "L2 conferência do bundle no job flatpak"
else falha "L2 o job flatpak não roda conferencia_bundle_libs.sh flatpak_app"; fi

if /usr/bin/grep -q -- 'conferencia_repo_flatpak.sh repo' "$JOB"; then ok "L3 conferência do repositório no job flatpak"
else falha "L3 o job flatpak não roda conferencia_repo_flatpak.sh repo"; fi

if /usr/bin/grep -q -- 'conferencia_repo_flatpak.sh' "$ESPELHO"; then ok "L4 espelho roda conferencia_repo_flatpak.sh"
else falha "L4 tools/espelho/job_flatpak.sh não roda conferencia_repo_flatpak.sh"; fi

echo "FALHAS=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE LIGACAO BUNDLE: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE LIGACAO BUNDLE: REPROVADO"
exit 1
