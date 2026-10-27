#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da trava de build_local.sh (flock aninhado, incidente de 25/10/2026: 18 min de deadlock).
# T1 caminho antigo: build_local.sh <distro> <preset>, sem trava prévia e sem marca. Deve terminar rc 0 em <= 60 s.
# T2 caminho novo: o chamador detém a trava e exporta NL_TRAVA_DETIDA=1; executar NÃO readquire. rc 0 em <= 60 s.
# O docker é um STUB que só registra as chamadas: o teste exercita a trava, não o contêiner.
# Usa NL_TMP_BASE próprio (pasta temporária): não toca /var/tmp/notalivre-heavy.lock real.
# Uso: autoteste_trava.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-trava.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin" "$BASE/tmp"
printf '#!/bin/sh\necho "docker $*" >> "%s/docker.log"\nexit 0\n' "$BASE" > "$BASE/bin/docker"
chmod +x "$BASE/bin/docker"
export PATH="$BASE/bin:$PATH" NL_TMP_BASE="$BASE/tmp" NL_FATIA=autoteste-trava
TRAVA="$NL_TMP_BASE/notalivre-heavy.lock"
FALHAS=0

veredito() {
    if [ "$2" -eq 0 ]; then echo "[VERDE] $1: rc=$2 em <= 60 s"; else echo "[VERMELHO] $1: rc=$2"; FALHAS=$((FALHAS + 1)); fi
}

# T1: caminho antigo (construir -> executar), sem trava prévia
timeout 60 bash "$RAIZ/tools/conteiner/build_local.sh" fedora44 debug > "$BASE/t1.log" 2>&1
rc1=$?
veredito "T1 caminho antigo sem trava prévia" "$rc1"

# T2: caminho novo: o chamador segura a trava e marca NL_TRAVA_DETIDA=1 (executar não pode readquirir)
timeout 60 flock "$TRAVA" env NL_TRAVA_DETIDA=1 bash "$RAIZ/tools/conteiner/build_local.sh" executar fedora44 debug > "$BASE/t2.log" 2>&1
rc2=$?
veredito "T2 caminho novo com trava detida pelo chamador" "$rc2"

# Conferência: os dois caminhos chegaram ao docker stub (prova de que não saíram cedo)
chamadas=$(grep -c '^docker ' "$BASE/docker.log" 2>/dev/null || true)
echo "chamadas_docker_stub=${chamadas:-0} (esperado >= 4: 2 por execução)"
[ "${chamadas:-0}" -ge 4 ] || FALHAS=$((FALHAS + 1))

echo "FALHAS=$FALHAS"
[ "$FALHAS" -eq 0 ] && { echo "AUTOTESTE TRAVA: todos os caminhos terminam"; exit 0; }
echo "AUTOTESTE TRAVA: REPROVADO"
exit 1
