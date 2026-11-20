#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do átomo do teto de contêiner (NL-3.K; L-36). Só shell: não sobe contêiner. Tabela de nl_memoria_valida e dos
# vetores de nl_cotas_docker (run e build), com nproc e a lista de CPUs online trocados por costura (NL_NPROC_TESTE, NL_CPUS_ONLINE_ARQ), para dar o
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

# pids VALOR RC VETOR SEM: nl_cotas_docker com NL_PIDS=VALOR (NL-3.1h, teto duro de processos). VETOR é o de run, com
# --pids-limit; SEM é o vetor sem --pids-limit, que o banco e o ensaio usam com o próprio literal. RC 2 deixa os dois vazios.
pids() {
    local obtido
    obtido="$( (NL_NPROC_TESTE=16; NL_PIDS="$1"; NL_CPUS_ONLINE_ARQ="$BASE/online15"
                nl_cotas_docker 2>/dev/null; echo "rc=$? vetor=${NL_COTAS_DOCKER[*]} sem=${NL_COTAS_DOCKER_SEM_PIDS[*]}") )"
    CASOS=$((CASOS + 1))
    if [ "$obtido" = "rc=$2 vetor=$3 sem=$4" ]; then
        echo "ok: pids [$1]: $obtido"
    else
        echo "REPROVA: pids [$1]: esperado [rc=$2 vetor=$3 sem=$4], obtido [$obtido]"
        FALHAS=$((FALHAS + 1))
    fi
}

# Costura NL_PIDS fora do ambiente do autoteste: o valor de cada caso é o que a linha do caso diz.
unset NL_PIDS
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
vetor 16 "" "" "" 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 400"
vetor 16 2 4g "" 0 "--cpus 2 --memory 4g --memory-swap 4g --pids-limit 400"
vetor 16 "" "" 8-11 0 "--cpuset-cpus 8-11 --cpus 7 --memory 12g --memory-swap 12g --pids-limit 400"
vetor 16 "" "" todas 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 400"
vetor 4 "" "" "" 0 "--cpus 3 --memory 12g --memory-swap 12g --pids-limit 400"
vetor 1 "" "" "" 0 "--cpus 1 --memory 12g --memory-swap 12g --pids-limit 400"
vetor 16 8 "" "" 2 ""
vetor 4 4 "" "" 2 ""
vetor 16 "" 13g "" 2 ""
vetor 16 "" "" 4-99 2 ""
vetor 16 "" "" "4-15;x" 2 ""
# NL-3.1h (T3): NL_PIDS vazia ou ausente vale 400; 1 a 400 sem zero à esquerda passam; o resto reprova com rc 2 e vetores vazios.
pids "" 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 400" "--cpus 7 --memory 12g --memory-swap 12g"
pids 128 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 128" "--cpus 7 --memory 12g --memory-swap 12g"
pids 400 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 400" "--cpus 7 --memory 12g --memory-swap 12g"
pids 1 0 "--cpus 7 --memory 12g --memory-swap 12g --pids-limit 1" "--cpus 7 --memory 12g --memory-swap 12g"
pids 401 2 "" ""
pids 0 2 "" ""
pids 0400 2 "" ""
pids abc 2 "" ""
pids 4e2 2 "" ""
pids "400;x" 2 "" ""
# vetor_build NPROC THREADS MEMORIA CPUSET RC VETOR: o mesmo, para o vetor do docker build (NL-3.K, revisão, A-1).
vetor_build() {
    local obtido
    obtido="$( (NL_NPROC_TESTE="$1"; NL_THREADS="$2"; NL_MEMORIA="$3"; NL_CPUSET="$4"; NL_CPUS_ONLINE_ARQ="$BASE/online15"
                nl_cotas_docker 2>/dev/null; echo "rc=$? vetor=${NL_COTAS_DOCKER_BUILD[*]}") )"
    CASOS=$((CASOS + 1))
    if [ "$obtido" = "rc=$5 vetor=$6" ]; then
        echo "ok: vetor_build nproc=[$1] threads=[$2] memoria=[$3] cpuset=[$4]: $obtido"
    else
        echo "REPROVA: vetor_build nproc=[$1] threads=[$2] memoria=[$3] cpuset=[$4]: esperado [rc=$5 vetor=$6], obtido [$obtido]"
        FALHAS=$((FALHAS + 1))
    fi
}
vetor_build 16 "" "" "" 0 "--cpu-period 100000 --cpu-quota 700000 --memory 12g --memory-swap 12g"
vetor_build 16 2 4g 8-11 0 "--cpuset-cpus 8-11 --cpu-period 100000 --cpu-quota 200000 --memory 4g --memory-swap 4g"
vetor_build 16 8 "" "" 2 ""
# Sem a costura, o número de CPUs é o da máquina (nproc --all), também dentro de um contêiner com --cpus (revisão, A-3).
todas="$(nproc --all)"
esperado=$((todas - 1))
[ "$esperado" -le "$NL_COTA_PADRAO" ] || esperado="$NL_COTA_PADRAO"
[ "$esperado" -ge 1 ] || esperado=1
obtido="$( (unset NL_NPROC_TESTE NL_THREADS NL_MEMORIA NL_CPUSET; nl_cotas_docker 2>/dev/null; echo "$NL_COTA_THREADS") )"
CASOS=$((CASOS + 1))
if [ "$obtido" = "$esperado" ]; then
    echo "ok: sem costura, nproc --all=$todas dá teto de CPU $obtido (nproc simples aqui: $(nproc))"
else
    echo "REPROVA: sem costura, nproc --all=$todas devia dar teto de CPU $esperado, obtido [$obtido] (nproc simples aqui: $(nproc))"
    FALHAS=$((FALHAS + 1))
fi
aviso="$( (NL_NPROC_TESTE=4; nl_cotas_docker 2>&1 >/dev/null) )"
CASOS=$((CASOS + 1))
if [[ "$aviso" == *"AVISO: máquina com 4 CPUs"* ]]; then
    echo "ok: máquina de 4 CPUs avisa a redução do teto"
else
    echo "REPROVA: máquina de 4 CPUs sem o AVISO da redução (stderr: [$aviso])"
    FALHAS=$((FALHAS + 1))
fi

echo "cota_conteiner: casos=$CASOS falharam=$FALHAS"
if [ "$FALHAS" -eq 0 ] && [ "$CASOS" -ge 27 ]; then
    echo "PASSA: teto de contêiner (átomo)"
    exit 0
fi
echo "REPROVA: teto de contêiner ($FALHAS falha(s), $CASOS casos)"
exit 1
