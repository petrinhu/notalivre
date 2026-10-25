#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Executor do espelho local (R2, A0-02): roda cada linha de tools/espelho_local.txt, UM POR VEZ,
# e imprime o rc de cada job com o log. Cada comando pega a própria trava flock (L-11).
# rc 0 = verde; rc 2 = não executado (lacuna declarada); qualquer outro = vermelho.
# Uso: rodar_espelho.sh          (saída: uma linha por job e o resumo; sai 1 se algum job não for verde)
set -u
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NL_TMP_BASE="${NL_TMP_BASE:-/var/tmp}"
export NL_TMP_BASE TMPDIR="$NL_TMP_BASE"
LISTA="$RAIZ/tools/espelho_local.txt"
LOGDIR="$NL_TMP_BASE/notalivre-logs"
SHA="$(git -C "$RAIZ" rev-parse --short HEAD)"
if [ -n "$(git -C "$RAIZ" status --porcelain)" ]; then
    SHA="$SHA+sujo"
fi
mkdir -p "$LOGDIR" || exit 1
n=0; verdes=0; lacunas=0; falhas=0
while IFS= read -r linha; do
    case "$linha" in ''|\#*) continue ;; esac
    nome="${linha%% | *}"
    cmd="${linha#* | }"
    n=$((n + 1))
    slug="$(printf '%s' "$nome" | tr -c 'A-Za-z0-9' '-' | cut -c1-40)"
    log="$LOGDIR/espelho-$(printf '%02d' "$n")-$slug-$SHA.log"
    ( cd "$RAIZ" && bash -c "$cmd" ) > "$log" 2>&1
    rc=$?
    echo "job=\"$nome\" rc=$rc log=$log"
    case "$rc" in
        0) verdes=$((verdes + 1)) ;;
        2) lacunas=$((lacunas + 1)) ;;
        *) falhas=$((falhas + 1)) ;;
    esac
done < "$LISTA"
echo "espelho: jobs=$n verdes=$verdes nao_executados=$lacunas falhas=$falhas sha=$SHA"
[ "$n" -gt 0 ] && [ "$falhas" -eq 0 ] && [ "$lacunas" -eq 0 ] && exit 0
exit 1
