#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do átomo do teto de contêiner (NL-3.K; L-36). Só shell: não sobe contêiner. Tabela de nl_memoria_valida e do vetor
# de nl_cotas_docker, com nproc e a lista de CPUs online trocados por costura (NL_NPROC_TESTE, NL_CPUS_ONLINE_ARQ), para dar o
# mesmo resultado aqui, no contêiner do espelho e no runner do CI. Tabela vazia reprova.
# Uso: bash tools/conteiner/autoteste_cota_conteiner.sh   (RAIZ = a árvore onde este script está)
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ATOMO="$RAIZ/tools/conteiner/cota_conteiner.sh"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-cota.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0
if [ ! -f "$ATOMO" ]; then
    echo "REPROVA: átomo ausente ($ATOMO)"
    exit 1
fi
. "$ATOMO"
printf '0-15\n' > "$BASE/online15"

# mem VALOR ok|reprova: a regra de nl_memoria_valida.
mem() {
    local rc
    nl_memoria_valida "$1" >/dev/null 2>&1
    rc=$?
    CASOS=$((CASOS + 1))
    if { [ "$2" = ok ] && [ "$rc" = 0 ]; } || { [ "$2" = reprova ] && [ "$rc" = 1 ]; }; then
        echo "ok: memoria [$1] $2"
    else
        echo "REPROVA: memoria [$1] esperado $2, rc $rc"
        FALHAS=$((FALHAS + 1))
    fi
}

# vetor NPROC THREADS MEMORIA CPUSET RC VETOR: nl_cotas_docker num subshell, com os valores dados (vazio = ausente).
vetor() {
    local obtido
    obtido="$( (NL_NPROC_TESTE="$1"; NL_THREADS="$2"; NL_MEMORIA="$3"; NL_CPUSET="$4"; NL_CPUS_ONLINE_ARQ="$BASE/online15"
                nl_cotas_docker 2>/dev/null; echo "rc=$? vetor=${NL_COTAS_DOCKER[*]}") )"
    CASOS=$((CASOS + 1))
    if [ "$obtido" = "rc=$5 vetor=$6" ]; then
        echo "ok: vetor nproc=[$1] threads=[$2] memoria=[$3] cpuset=[$4]: $obtido"
    else
        echo "REPROVA: vetor nproc=[$1] threads=[$2] memoria=[$3] cpuset=[$4]: esperado [rc=$5 vetor=$6], obtido [$obtido]"
        FALHAS=$((FALHAS + 1))
    fi
}

mem "" ok
mem 12g ok
mem 4096m ok
mem 12288m ok
mem 13g reprova
mem 12289m reprova
mem 0g reprova
mem 08g reprova
mem 12G reprova
mem "12g;x" reprova
mem 99999999g reprova
vetor 16 "" "" "" 0 "--cpus 7 --memory 12g --memory-swap 12g"
vetor 16 2 4g "" 0 "--cpus 2 --memory 4g --memory-swap 4g"
vetor 16 "" "" 8-11 0 "--cpuset-cpus 8-11 --cpus 7 --memory 12g --memory-swap 12g"
vetor 16 "" "" todas 0 "--cpus 7 --memory 12g --memory-swap 12g"
vetor 4 "" "" "" 0 "--cpus 3 --memory 12g --memory-swap 12g"
vetor 1 "" "" "" 0 "--cpus 1 --memory 12g --memory-swap 12g"
vetor 16 8 "" "" 2 ""
vetor 4 4 "" "" 2 ""
vetor 16 "" 13g "" 2 ""
vetor 16 "" "" 4-99 2 ""
vetor 16 "" "" "4-15;x" 2 ""
aviso="$( (NL_NPROC_TESTE=4; nl_cotas_docker 2>&1 >/dev/null) )"
CASOS=$((CASOS + 1))
if [[ "$aviso" == *"AVISO: máquina com 4 CPUs"* ]]; then
    echo "ok: máquina de 4 CPUs avisa a redução do teto"
else
    echo "REPROVA: máquina de 4 CPUs sem o AVISO da redução (stderr: [$aviso])"
    FALHAS=$((FALHAS + 1))
fi

echo "cota_conteiner: casos=$CASOS falharam=$FALHAS"
if [ "$FALHAS" -eq 0 ] && [ "$CASOS" -ge 23 ]; then
    echo "PASSA: teto de contêiner (átomo)"
    exit 0
fi
echo "REPROVA: teto de contêiner ($FALHAS falha(s), $CASOS casos)"
exit 1
