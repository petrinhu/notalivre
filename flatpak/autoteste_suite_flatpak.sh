#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste de estreia de cada ramo do suite_flatpak.sh (NL-2.11, DA-2.61 A4, A9 e A9b; L-36). Um ctest de mentira
# (STUB no PATH, nenhum teste roda) devolve a contagem, o resumo e o rc de cada caso. Cada caso tem de dar o rc e a
# mensagem do ramo que ele exercita. Um ramo sem teste que o exercite é verde falso (L-36).
# Uso: autoteste_suite_flatpak.sh     (usa o suite_flatpak.sh da árvore onde este arquivo está)
set -u

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASE="$(mktemp -d "${TMPDIR:-/var/tmp}/notalivre-autoteste-suite.XXXXXX")" || exit 1
trap 'rm -rf -- "$BASE"' EXIT
mkdir -p "$BASE/bin"
cat > "$BASE/bin/ctest" <<'STUB'
#!/bin/sh
case "$*" in
    *" -N"*) echo "  Test  #1: stub"; echo "Total Tests: ${STUB_TOTAL:-0}"; exit 0 ;;
esac
if [ -n "${STUB_RESUMO:-}" ]; then
    echo "$STUB_RESUMO"
    echo "Total Test time (real) =   0.01 sec"
fi
exit "${STUB_RC:-0}"
STUB
chmod +x "$BASE/bin/ctest"
export PATH="$BASE/bin:$PATH"

CASOS=0
FALHAS=0

# caso NOME PISO MAIS TOTAL RESUMO RC ESPERADO_RC TEXTO
# MAIS vazio = sem flatpak/piso_mais.txt. RESUMO vazio = saída sem a linha "tests passed".
caso() {
    local nome="$1" piso="$2" mais="$3" total="$4" resumo="$5" rc="$6" esperado="$7" texto="$8"
    local dir="$BASE/c-$nome" saida rc_suite
    CASOS=$((CASOS + 1))
    mkdir -p "$dir/flatpak" "$dir/_build" "$dir/tools/portoes" "$dir/tests" "$dir/_build/junit"
    # NL-2.14: o suite chama o portão de pulos depois das checagens acima. O caso verde precisa de um junitxml válido
    # (um teste, sem pulo) e da lista real, senão o portão reprova um caso que só quer testar a contagem.
    cp "$RAIZ/tools/portoes/pulos_testes.sh" "$dir/tools/portoes/pulos_testes.sh"
    cp "$RAIZ/tests/pulos_esperados_por_distro.txt" "$dir/tests/pulos_esperados_por_distro.txt"
    printf '%s\n' stub > "$dir/_build/junit/esperados.txt"
    printf '%s\n' '<?xml version="1.0" encoding="UTF-8" ?>' '<testsuite name="TstStub" tests="1" failures="0" errors="0" skipped="0" time="0.001">' \
        '<testcase name="stub" classname="TstStub" time="0.000"/>' '</testsuite>' > "$dir/_build/junit/stub.xml"
    cp "$RAIZ/flatpak/suite_flatpak.sh" "$dir/flatpak/suite_flatpak.sh"
    printf '%s\n' "$piso" > "$dir/flatpak/piso_suite.txt"
    if [ -n "$mais" ]; then
        printf '%s\n' "$mais" > "$dir/flatpak/piso_mais.txt"
    fi
    saida="$(cd "$dir" && STUB_TOTAL="$total" STUB_RESUMO="$resumo" STUB_RC="$rc" timeout 120 sh flatpak/suite_flatpak.sh _build 2>&1)"
    rc_suite=$?
    echo "--- caso $nome: rc=$rc_suite (esperado $esperado)"
    echo "$saida" | grep -E '^(PARADA|suite_flatpak)' | tail -n 2
    if [ "$rc_suite" = "$esperado" ] && printf '%s\n' "$saida" | grep -q -F -- "$texto"; then
        echo "[VERDE] $nome"
    else
        echo "[VERMELHO] $nome: rc=$rc_suite, esperava rc=$esperado com '$texto'"
        FALHAS=$((FALHAS + 1))
    fi
}

OK3="100% tests passed, 0 tests failed out of 3"
OK4="100% tests passed, 0 tests failed out of 4"
OK2="100% tests passed, 0 tests failed out of 2"
FALHA3="67% tests passed, 1 tests failed out of 3"

# Controle positivo: contagem exata, tudo passa.
caso verde 3 "" 3 "$OK3" 0 0 "VERDE, 3 de 3 testes passaram, piso 3"
# Formato curto do ctest do SDK (run 41182301920): "100% tests passed out of 3", sem a parte ", 0 tests failed". Zero falha
# é verde, não falharam=1 por omissão (o job flatpak reprovou um run inteiro de 236 testes verdes por isso).
caso verde_formato_curto 3 "" 3 "100% tests passed out of 3" 0 0 "VERDE, 3 de 3 testes passaram, piso 3"
# Ramo do piso +1 (manifesto): piso 4, o CTest lista 3.
caso piso_mais_mais1 3 1 3 "$OK3" 0 1 "o CTest registrou 3 testes, abaixo do piso 4"
# Contagem exata para CIMA: teste novo registrado sem atualizar o piso (era o buraco do -lt).
caso acima_do_piso 3 "" 4 "$OK4" 0 1 "o CTest registrou 4 testes, acima do piso 3"
# Contagem exata para BAIXO: teste perdido.
caso abaixo_do_piso 3 "" 2 "$OK2" 0 1 "o CTest registrou 2 testes, abaixo do piso 3"
# Ramo falharam com rc 0 (o ctest devolve 0 com falha no resumo): só o ramo falharam pega.
caso falharam_rc0 3 "" 3 "$FALHA3" 0 1 "1 teste(s) falharam (ctest rc=0)"
# Ramo falharam com rc diferente de 0: os dois ramos apontam, o mais forte é a falha de verdade.
caso falharam_rc8 3 "" 3 "$FALHA3" 8 1 "1 teste(s) falharam (ctest rc=8)"
# Ramo rc isolado: rc 8 sem falha no resumo (só o ramo do rc pega).
caso rc_sem_falha 3 "" 3 "$OK3" 8 1 "0 teste(s) falharam (ctest rc=8)"
# Ramo analisados: o resumo diz que executou 2 de 3 listados (varredura incompleta), mesmo com rc 0 e 0 falhas.
caso analisados_menor 3 "" 3 "$OK2" 0 1 "varredura incompleta: executou 2 de 3"
# Ramo analisados com resumo ausente: analisados vira 0.
caso sem_resumo 3 "" 3 "" 0 1 "varredura incompleta: executou 0 de 3"
# Piso 0 recusado (A9b): zero testes não pode passar.
caso piso_zero 0 "" 0 "" 0 1 "piso 0 recusado"
# Piso ilegível.
caso piso_ilegivel "3 " "" 3 "$OK3" 0 1 "piso ilegível"

echo "casos=$CASOS falhas=$FALHAS"
if [ "$FALHAS" -eq 0 ]; then
    echo "AUTOTESTE SUITE FLATPAK: todos os casos verdes"
    exit 0
fi
echo "AUTOTESTE SUITE FLATPAK: REPROVADO"
exit 1
