#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste da cota de núcleos (NL-3.0k, ordem do CTO, item 6). Só shell e grep: não sobe contêiner e não roda flatpak.
# Três partes, todas precisam passar:
#   (1) tabela da regra do átomo tools/conteiner/cota_nucleos.sh, com NPROC 16 fixo;
#   (2) o job_flatpak.sh e o sabotar_portoes.sh carregam o átomo, passam --jobs e não têm regex própria;
#   (3) o manifesto tem --parallel e nenhum `cmake --build _build` puro; o padrão do build_local.sh é o do átomo.
# Registro: job_autotestes.sh e o job autotestes do ci.yml chamam este script. Tabela vazia reprova.
# Uso: bash tools/espelho/autoteste_cota_nucleos.sh   (RAIZ = a árvore onde este script está)
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ATOMO="$RAIZ/tools/conteiner/cota_nucleos.sh"
JOB="$RAIZ/tools/espelho/job_flatpak.sh"
SABOTAR="$RAIZ/tools/mutacao/sabotar_portoes.sh"
MANIFESTO="$RAIZ/flatpak/io.github.petrinhu.notalivre.yml"
BUILD_LOCAL="$RAIZ/tools/conteiner/build_local.sh"
JOB_AUTOTESTES="$RAIZ/tools/espelho/job_autotestes.sh"
CI="$RAIZ/.github/workflows/ci.yml"
TOTAL=0
CASOS=0
FALHAS=0

checar() {
    local nome="$1"; shift
    TOTAL=$((TOTAL + 1))
    if "$@" >/dev/null 2>&1; then
        echo "ok: $nome"
    else
        echo "REPROVA: $nome"
        FALHAS=$((FALHAS + 1))
    fi
}

# caso VALOR ok|reprova: a regra do átomo com NPROC 16 tem de dar o veredito esperado.
caso() {
    local valor="$1" esperado="$2" rc
    nl_threads_valido "$valor" 16 >/dev/null 2>&1
    rc=$?
    CASOS=$((CASOS + 1))
    TOTAL=$((TOTAL + 1))
    if { [ "$esperado" = ok ] && [ "$rc" -eq 0 ]; } || { [ "$esperado" = reprova ] && [ "$rc" -eq 1 ]; }; then
        echo "ok: caso NL_THREADS=[$valor] $esperado (rc $rc)"
    else
        echo "REPROVA: caso NL_THREADS=[$valor] esperado $esperado, rc $rc"
        FALHAS=$((FALHAS + 1))
    fi
}

sem_regex() { ! grep -q -- '=~' "$1"; }
carrega_atomo() { grep -q -- 'conteiner/cota_nucleos.sh' "$1"; }
passa_jobs() { grep -q -- '--jobs="\$JOBS"' "$1"; }
tem_conferencia() { grep -q -- 'grep -qx "jobs-do-build=\$JOBS"' "$1"; }
tem_parallel() { grep -q -- '--parallel "\${FLATPAK_BUILDER_N_JOBS:-\$(nproc)}"' "$1"; }
sem_build_puro() {
    local puros
    puros="$(grep -n 'cmake --build _build' "$1" | grep -v -- '--parallel' || true)"
    [ -z "$puros" ]
}
registrado() { grep -q -- 'autoteste_cota_nucleos.sh' "$1"; }
padrao_igual() { [ -n "$1" ] && [ "$1" = "$2" ]; }
aviso_acima_da_cota() { nl_threads_valido 8 16 2>&1 >/dev/null | grep -q AVISO; }

echo "## Parte 1: tabela da regra (átomo, NPROC 16)"
checar "átomo existe e carrega" test -f "$ATOMO"
if [ -f "$ATOMO" ]; then
    . "$ATOMO"
    caso "" ok
    caso 1 ok
    caso 7 ok
    caso 8 ok
    caso 15 ok
    caso 0 reprova
    caso 07 reprova
    caso -1 reprova
    caso abc reprova
    caso "7;x" reprova
    caso 16 reprova
    caso 1000000 reprova
    caso 99999999999999999999 reprova
    checar "tabela não vazia" [ "$CASOS" -ge 1 ]
    checar "8 (acima da cota) emite AVISO" aviso_acima_da_cota
fi

echo "## Parte 2: chamadores carregam o átomo, passam --jobs e não têm regex própria"
for f in "$JOB" "$SABOTAR"; do
    nome="$(basename "$f")"
    checar "$nome carrega o átomo" carrega_atomo "$f"
    checar "$nome passa --jobs=\"\$JOBS\"" passa_jobs "$f"
    checar "$nome sem regex própria" sem_regex "$f"
done
checar "job_flatpak.sh confere jobs-do-build no log" tem_conferencia "$JOB"

echo "## Parte 3: manifesto e padrão do build_local.sh"
checar "manifesto tem --parallel com nproc de reserva" tem_parallel "$MANIFESTO"
checar "manifesto sem cmake --build _build puro" sem_build_puro "$MANIFESTO"
padrao_build="$(sed -n 's/^THREADS="\${NL_THREADS:-\([0-9]*\)}"$/\1/p' "$BUILD_LOCAL")"
padrao_atomo="$(sed -n 's/^NL_COTA_PADRAO=\([0-9]*\)$/\1/p' "$ATOMO" 2>/dev/null)"
checar "padrão do build_local.sh ($padrao_build) igual ao do átomo ($padrao_atomo)" padrao_igual "$padrao_build" "$padrao_atomo"

echo "## Parte 4: registro no espelho e no CI"
checar "job_autotestes.sh chama o autoteste" registrado "$JOB_AUTOTESTES"
checar "ci.yml chama o autoteste" registrado "$CI"

echo "cota_nucleos: verificacoes=$TOTAL casos=$CASOS falharam=$FALHAS"
if [ "$FALHAS" -eq 0 ] && [ "$CASOS" -ge 1 ]; then
    echo "PASSA: cota de núcleos (átomo, chamadores, manifesto, build_local, registro)"
    exit 0
fi
echo "REPROVA: cota de núcleos ($FALHAS falha(s))"
exit 1
