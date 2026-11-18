#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do harness de integração com banco (NL-3.1h, V-a; L-35, L-36). Vermelho antes do harness: integracao_banco.sh
# ainda não existe, então cada caso reprova. Um caso só passa com a saída e o rc que o harness deve dar.
# Não sobe contêiner: o docker é um de mentira no PATH, e nenhum servidor nem executor sobe.
# Casos (9): servidor ausente; zero testes banco; build sem NL_TESTES_BANCO; junitxml do par anterior; junitxml com
# skipped; limpeza em falha; docker run sem o vetor da cota; par fora da matriz; arquivo de banco/ executado direto.
# Uso: autoteste_integracao_banco.sh
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
CONTEINER="$RAIZ/tools/conteiner"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-banco.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
FALHAS=0
TOTAL=0
SHA_FIXO=0000000000000000000000000000000000000000

ok() { echo "[VERDE] $1"; }
falha() { echo "[VERMELHO] $1"; FALHAS=$((FALHAS + 1)); }

mkdir -p "$BASE/bin"
cat > "$BASE/bin/docker" <<'STUB'
#!/bin/sh
echo "docker $*" >> "${STUB_LOG:-/dev/null}"
case "$1" in
    image) [ -n "${STUB_SERVIDOR_AUSENTE:-}" ] && exit 1; exit 0 ;;
    run) exit "${STUB_RUN_RC:-0}" ;;
esac
exit 0
STUB
chmod +x "$BASE/bin/docker"

# Prepara um caso novo: pasta própria, build com a opção ligada e junitxml vazio. Nada é apagado aqui.
novo_caso() {
    CASO="$BASE/casos/$1"
    mkdir -p "$CASO/tmp" || return 1
    NL_TMP_BASE="$CASO/tmp"
    BUILD="$NL_TMP_BASE/notalivre-build-fedora45-NL-3.1h/banco"
    mkdir -p "$BUILD/junit" || return 1
    printf 'NL_TESTES_BANCO:BOOL=ON\n' > "$BUILD/CMakeCache.txt"
    STUB_LOG="$CASO/docker.log"
    export NL_TMP_BASE STUB_LOG TMPDIR="$CASO/tmp" NL_FATIA=NL-3.1h NL_SHA="$SHA_FIXO"
    unset STUB_SERVIDOR_AUSENTE STUB_RUN_RC
    export PATH="$BASE/bin:$PATH"
}

# Roda o comando e exige rc diferente de 0 e o trecho na saída (o harness precisa dar PARADA com a mensagem certa).
executar_caso() {
    local nome="$1" trecho="$2" saida rc
    shift 2
    TOTAL=$((TOTAL + 1))
    saida="$("$@" 2>&1)"; rc=$?
    if [ "$rc" -ne 0 ] && [[ "$saida" == *"$trecho"* ]]; then
        ok "$nome (rc=$rc)"
    else
        falha "$nome: rc=$rc, esperado rc diferente de 0 e '$trecho' na saída"
    fi
}

# Caso 1: servidor ausente reprova (a imagem do servidor não está no host).
novo_caso servidor_ausente || { echo "ABORTA: caso 1"; exit 1; }
export STUB_SERVIDOR_AUSENTE=1
executar_caso "servidor ausente reprova" "PARADA" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 2: zero testes banco reprova (nenhum junitxml de integracao.banco.* no build).
novo_caso zero_testes || { echo "ABORTA: caso 2"; exit 1; }
executar_caso "label banco com zero testes reprova" "PARADA" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 3: build sem NL_TESTES_BANCO:BOOL=ON reprova (mutante 25).
novo_caso build_sem_opcao || { echo "ABORTA: caso 3"; exit 1; }
printf 'NL_TESTES_BANCO:BOOL=OFF\n' > "$BUILD/CMakeCache.txt"
executar_caso "build sem NL_TESTES_BANCO reprova" "PARADA" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 4: junitxml do par anterior não conta como resultado do par atual (mutante 28).
novo_caso junit_do_par_anterior || { echo "ABORTA: caso 4"; exit 1; }
printf '<testsuite tests="1"><testcase classname="integracao.banco" name="conexaoBasica"/></testsuite>\n' \
    > "$BUILD/junit/integracao.banco.mysql97.xml"
executar_caso "junitxml do par anterior não conta" "PARADA" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 5: junitxml com skipped reprova (veto da v3.1: nenhum teste do banco pulado).
novo_caso junit_com_skipped || { echo "ABORTA: caso 5"; exit 1; }
printf '<testsuite tests="1"><testcase classname="integracao.banco" name="conexaoBasica"><skipped/></testcase></testsuite>\n' \
    > "$BUILD/junit/integracao.banco.mysql97.xml"
executar_caso "junitxml com skipped reprova" "pulados=1" bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97

# Caso 6: a limpeza roda em falha. Com o executor vermelho (rc 1), o harness tem de chamar docker stop ou docker rm.
novo_caso limpeza_em_falha || { echo "ABORTA: caso 6"; exit 1; }
export STUB_RUN_RC=1
TOTAL=$((TOTAL + 1))
bash "$CONTEINER/integracao_banco.sh" fedora45 mysql97 >/dev/null 2>&1; rc=$?
if [ "$rc" -ne 0 ] && grep -Eq 'docker (stop|rm)' "$CASO/docker.log" 2>/dev/null; then
    ok "limpeza roda em falha (rc=$rc)"
else
    falha "limpeza roda em falha: rc=$rc, sem docker stop ou docker rm no log"
fi

# Caso 7: docker run sem o vetor da cota reprova. Varredura: todo docker run do harness e de banco/ leva o vetor.
# Piso (L-36): zero docker run encontrado é varredura quebrada, e reprova.
TOTAL=$((TOTAL + 1))
total_run=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null | wc -l)
sem_vetor=$(grep -hF 'docker run' "$CONTEINER/integracao_banco.sh" "$CONTEINER"/banco/*.sh 2>/dev/null \
    | grep -vF 'NL_COTAS_DOCKER[@]' | wc -l)
echo "docker_run_sem_vetor: docker_run=$total_run sem_vetor=$sem_vetor"
if [ "$total_run" -gt 0 ] && [ "$sem_vetor" -eq 0 ]; then
    ok "docker run do harness leva o vetor da cota ($total_run chamadas)"
else
    falha "docker run do harness sem o vetor, ou varredura vazia (docker_run=$total_run sem_vetor=$sem_vetor)"
fi

# Caso 8: par fora da matriz reprova (ubuntu26.04 tem só mysql84 e mariadb118 em matriz_banco.txt).
novo_caso par_fora_da_matriz || { echo "ABORTA: caso 8"; exit 1; }
executar_caso "par fora da matriz reprova" "PARADA" bash "$CONTEINER/integracao_banco.sh" ubuntu26.04 mysql97

# Caso 9: arquivo de banco/ executado direto sai com rc diferente de 0 (são carregados por ponto).
novo_caso banco_direto || { echo "ABORTA: caso 9"; exit 1; }
executar_caso "arquivo de banco/ executado direto reprova" "PARADA" bash "$CONTEINER/banco/listas.sh"

echo "casos=$TOTAL falharam=$FALHAS"
if [ "$TOTAL" -ne 9 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: contagem de casos diferente de 9"
    exit 1
fi
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE INTEGRACAO BANCO: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE INTEGRACAO BANCO: REPROVADO"
exit 1
