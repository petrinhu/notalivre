#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do paralelismo explícito e da medição de processos dentro do contêiner (NL-3.1h, emenda v3.11; L-36).
# Roda DENTRO do contêiner (cwd /src, a árvore do SHA em ro), com cmake e ctest de mentira no PATH. Não sobe contêiner.
#   T4  executar_no_conteiner.sh passa --parallel "$CMAKE_BUILD_PARALLEL_LEVEL" ao build; valor fora de 1 a 7 é PARADA, rc 2,
#       sem nenhuma chamada de cmake --build.
#   T5  ensaio_no_conteiner.sh faz o mesmo, depois da checagem de RLIMIT_CORE e de pids.max.
#   T6  nl_pids_linha (tools/conteiner/medir_pids_conteiner.sh) imprime NL-PIDS com os arquivos do cgroup, ou ausente;
#       e o executar imprime a linha com o rc do build vermelho preservado.
#   T7  no preset banco, o rc do portão de funções é o rc do script (NL-3.1h, v3.16).
# Uso: bash tools/conteiner/autoteste_paralelo_conteiner.sh      (no contêiner; RAIZ = cwd)
set -u
RAIZ="$(pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-paralelo.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
CASOS=0
FALHAS=0

ok() { CASOS=$((CASOS + 1)); echo "ok: $1"; }
falha() { CASOS=$((CASOS + 1)); FALHAS=$((FALHAS + 1)); echo "REPROVA: $1"; }

# Ferramentas de mentira: cmake e ctest gravam o argv no arquivo de CMAKE_LOG e saem com FAKE_BUILD_RC no --build.
mkdir -p "$BASE/bin" "$BASE/tmp"
cat > "$BASE/bin/cmake" <<'STUB'
#!/bin/sh
echo "$*" >> "$CMAKE_LOG"
if [ "$1" = --build ]; then
    exit "${FAKE_BUILD_RC:-0}"
fi
exit 0
STUB
cat > "$BASE/bin/ctest" <<'STUB'
#!/bin/sh
exit 0
STUB
chmod +x "$BASE/bin/cmake" "$BASE/bin/ctest"

# rodar_executar PARALELO [RC_BUILD]: roda o corpo do executar com o PARALELO dado; devolve o rc em RC e a saída em SAIDA.
# Costura: o argv do cmake vai para $BASE/cmake.argv; o arquivo é zerado antes de cada caso.
rodar_executar() {
    : > "$BASE/cmake.argv"
    local paralelo="$1" rc_build="${2:-0}"
    if [ "$paralelo" = AUSENTE ]; then
        SAIDA="$(env -u CMAKE_BUILD_PARALLEL_LEVEL PATH="$BASE/bin:$PATH" CMAKE_LOG="$BASE/cmake.argv" FAKE_BUILD_RC="$rc_build" \
            NL_TMP_BASE="$BASE/tmp" TMPDIR="$BASE/tmp" bash "$RAIZ/tools/conteiner/executar_no_conteiner.sh" debug 2>&1)"
    else
        SAIDA="$(PATH="$BASE/bin:$PATH" CMAKE_LOG="$BASE/cmake.argv" FAKE_BUILD_RC="$rc_build" CMAKE_BUILD_PARALLEL_LEVEL="$paralelo" \
            NL_TMP_BASE="$BASE/tmp" TMPDIR="$BASE/tmp" bash "$RAIZ/tools/conteiner/executar_no_conteiner.sh" debug 2>&1)"
    fi
    RC=$?
}

# T4: paralelo válido chega ao build; inválido sai com rc 2 e zero --build.
rodar_executar 5
if grep -q -x -F -- '--build --preset debug --parallel 5' "$BASE/cmake.argv"; then
    ok "T4 executar: cmake --build com --parallel 5"
else
    falha "T4 executar: esperava '--build --preset debug --parallel 5' no argv do cmake (argv: $(tr '\n' '|' < "$BASE/cmake.argv"))"
fi
for ruim in AUSENTE "" 0 8 x 10 17; do
    rodar_executar "$ruim"
    if [ "$RC" = 2 ] && ! grep -q -- '^--build' "$BASE/cmake.argv"; then
        ok "T4 executar: paralelo [$ruim] é PARADA, rc 2 e zero --build"
    else
        falha "T4 executar: paralelo [$ruim] esperava rc 2 e zero --build (rc=$RC, argv: $(tr '\n' '|' < "$BASE/cmake.argv"))"
    fi
done

# T7: preset banco. O portão de funções (python3 de mentira, só aqui) decide o rc do script, e a linha NL-BANCO-BUILD sai.
mkdir -p "$BASE/binpy"
printf '#!/bin/sh\nexit "${FAKE_PY_RC:-0}"\n' > "$BASE/binpy/python3"
chmod +x "$BASE/binpy/python3"
for rc_py in 0 1; do
    : > "$BASE/cmake.argv"
    SAIDA="$(PATH="$BASE/binpy:$BASE/bin:$PATH" CMAKE_LOG="$BASE/cmake.argv" FAKE_PY_RC="$rc_py" CMAKE_BUILD_PARALLEL_LEVEL=2 \
        NL_TMP_BASE="$BASE/tmp" TMPDIR="$BASE/tmp" bash "$RAIZ/tools/conteiner/executar_no_conteiner.sh" banco 2>&1)"
    RC=$?
    if [ "$RC" = "$rc_py" ] && [[ "$SAIDA" == *"NL-BANCO-BUILD: configure e build; ctest só no harness"* ]]; then
        ok "T7 banco: portão de funções com rc $rc_py, script saiu com $RC"
    else
        falha "T7 banco: portão de funções com rc $rc_py, script saiu com $RC (esperado $rc_py)"
    fi
done

# T6a: nl_pids_linha lê max, peak e eventos_max de NL_ARQ_CGROUP; arquivo ausente vira ausente.
. "$RAIZ/tools/conteiner/medir_pids_conteiner.sh" 2>/dev/null || true
mkdir -p "$BASE/cg1" "$BASE/cg2" "$BASE/cg3"
printf '400\n' > "$BASE/cg1/pids.max"
printf '123\n' > "$BASE/cg1/pids.peak"
printf 'max 0\n' > "$BASE/cg1/pids.events"
printf '400\n' > "$BASE/cg3/pids.max"
printf '77\n' > "$BASE/cg3/pids.peak"
printf 'max 7\n' > "$BASE/cg3/pids.events"
if [ "$(NL_ARQ_CGROUP="$BASE/cg1" nl_pids_linha 2>/dev/null)" = 'NL-PIDS max=400 peak=123 eventos_max=0' ]; then
    ok "T6 nl_pids_linha: max, peak e eventos_max lidos do cgroup"
else
    falha "T6 nl_pids_linha: esperava 'NL-PIDS max=400 peak=123 eventos_max=0'"
fi
if [ "$(NL_ARQ_CGROUP="$BASE/cg2" nl_pids_linha 2>/dev/null)" = 'NL-PIDS max=ausente peak=ausente eventos_max=ausente' ]; then
    ok "T6 nl_pids_linha: cgroup sem arquivos vira ausente"
else
    falha "T6 nl_pids_linha: cgroup vazio devia dar ausente nos três campos"
fi
if [ "$(NL_ARQ_CGROUP="$BASE/cg3" nl_pids_linha 2>/dev/null)" = 'NL-PIDS max=400 peak=77 eventos_max=7' ]; then
    ok "T6 nl_pids_linha: eventos_max acima de zero é lido, não escondido"
else
    falha "T6 nl_pids_linha: esperava eventos_max=7 com pids.events 'max 7'"
fi

# T6b: o executar imprime a linha NL-PIDS mesmo com o build vermelho, e o rc 3 do build é preservado.
rodar_executar 5 3
if [ "$RC" = 3 ] && printf '%s\n' "$SAIDA" | grep -q -E '^NL-PIDS max=.* peak=.* eventos_max='; then
    ok "T6 executar: build vermelho sai com rc 3 e imprime NL-PIDS"
else
    falha "T6 executar: esperava rc 3 e a linha NL-PIDS com o build vermelho (rc=$RC)"
fi

# T5: ensaio, mesma guarda antes do build. Costuras do autoteste: limites e pids.max em arquivo.
printf 'Max core file size            1                    1                    bytes\n' > "$BASE/limites"
printf '400\n' > "$BASE/pids.max"
ensaio() {
    : > "$BASE/cmake.argv"
    ENSAIO_RC="$(env -u CMAKE_BUILD_PARALLEL_LEVEL PATH="$BASE/bin:$PATH" CMAKE_LOG="$BASE/cmake.argv" FAKE_BUILD_RC=3 \
        NL_TMP_BASE="$BASE/tmp" TMPDIR="$BASE/tmp" NL_ENSAIO='x:1' NL_ENSAIO_SHA=0123456789abcdef0123456789abcdef01234567 \
        NL_ENSAIO_ARQ_LIMITES="$BASE/limites" NL_ENSAIO_ARQ_PIDS_MAX="$BASE/pids.max" "$@" bash "$RAIZ/tools/conteiner/ensaio_no_conteiner.sh" debug >/dev/null 2>&1; echo $?)"
}
ensaio CMAKE_BUILD_PARALLEL_LEVEL=5
if grep -q -x -F -- '--build --preset debug --parallel 5' "$BASE/cmake.argv"; then
    ok "T5 ensaio: cmake --build com --parallel 5"
else
    falha "T5 ensaio: esperava '--build --preset debug --parallel 5' no argv do cmake (argv: $(tr '\n' '|' < "$BASE/cmake.argv"))"
fi
ensaio CMAKE_BUILD_PARALLEL_LEVEL=x
if [ "$ENSAIO_RC" = 2 ] || [ "$ENSAIO_RC" = 5 ]; then
    if ! grep -q -- '^--build' "$BASE/cmake.argv"; then
        ok "T5 ensaio: paralelo x é PARADA antes do build"
    else
        falha "T5 ensaio: paralelo x chegou ao cmake --build"
    fi
else
    falha "T5 ensaio: paralelo x esperava rc 2 ou 5 (rc=$ENSAIO_RC)"
fi

echo "casos=$CASOS falharam=$FALHAS"
if [ "$FALHAS" = 0 ] && [ "$CASOS" -gt 0 ]; then
    echo "AUTOTESTE PARALELO CONTEINER: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE PARALELO CONTEINER: REPROVADO"
exit 1
