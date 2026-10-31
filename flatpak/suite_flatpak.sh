#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Suíte de unidade do build Flatpak (NL-2.11, DA-2.60 item 1; L-36). Roda, no sandbox do SDK, todos os testes que o CMake
# registra com NL_FLATPAK_BUILD=ON. Exclusão escrita (DA-2.28 e plano NL-2.11): fora do Flatpak ficam o servidor falso
# (tests/tools/servidor_falso, precisa de python, xmlsec1 e do arnês), o cliente Sefin (tests/unit/mid/sefin/cliente, precisa
# do servidor com oráculo) e os slots de xmldsig que chamam xmlsec1. Essas exclusões estão no próprio CMake, por NL_FLATPAK_BUILD.
#
# Piso versionado em flatpak/piso_suite.txt (1ª linha). A contagem é EXATA (DA-2.61, achado A4). Reprova se:
#   - o CTest listar número de testes diferente do piso: para menos (teste perdido) ou para mais (teste novo sem
#     atualizar o piso). A checagem roda ANTES do ctest que executa os testes, então sai sem rodar nenhum;
#   - o piso for 0 ou ilegível (zero testes não é varredura);
#   - algum teste falhar, ou o CTest não executar todos os que listou (varredura incompleta).
# Sabotagem de estreia do manifesto: flatpak/piso_mais.txt soma +1 ao piso; o disparo tem de sair pelo motivo do piso.
#
# Uso: suite_flatpak.sh BUILD      (cwd: raiz do código; BUILD é o diretório do CMake, _build no manifesto)
set -u

BUILD="${1:?uso: suite_flatpak.sh BUILD}"
PISO_ARQ="flatpak/piso_suite.txt"
MAIS_ARQ="flatpak/piso_mais.txt"

piso_base="$(sed -n '1p' "$PISO_ARQ" 2>/dev/null)"
case "$piso_base" in
    ''|*[!0-9]*) echo "PARADA: piso ilegível em $PISO_ARQ ('$piso_base')"; exit 1 ;;
esac
if [ "$piso_base" -eq 0 ]; then
    echo "PARADA: piso 0 recusado em $PISO_ARQ (zero testes não é varredura)"
    exit 1
fi
mais="$(sed -n '1p' "$MAIS_ARQ" 2>/dev/null)"
case "$mais" in
    ''|*[!0-9]*) mais=0 ;;
esac
piso=$((piso_base + mais))

ctest --test-dir "$BUILD" -N > "$BUILD/suite-listados.txt" 2>&1
encontrados="$(sed -n 's/^Total Tests: //p' "$BUILD/suite-listados.txt")"
case "$encontrados" in
    ''|*[!0-9]*) echo "PARADA: ctest -N não deu a contagem ('$encontrados')"; exit 1 ;;
esac

echo "suite_flatpak: piso=$piso (base $piso_base + mais $mais) encontrados=$encontrados"
if [ "$encontrados" -ne "$piso" ]; then
    if [ "$encontrados" -lt "$piso" ]; then sentido="abaixo do piso"; else sentido="acima do piso"; fi
    echo "PARADA: o CTest registrou $encontrados testes, $sentido $piso (teste perdido ou não registrado; teste novo pede atualizar $PISO_ARQ)"
    exit 1
fi

ctest --test-dir "$BUILD" -j 4 --output-on-failure > "$BUILD/suite-executados.txt" 2>&1
rc=$?
resumo="$(grep 'tests passed' "$BUILD/suite-executados.txt" | tail -n 1)"
analisados="$(printf '%s\n' "$resumo" | sed -n 's/.*out of \([0-9]*\).*/\1/p')"
falharam="$(printf '%s\n' "$resumo" | sed -n 's/.*, \([0-9]*\) tests failed.*/\1/p')"
[ -n "$analisados" ] || analisados=0
# O ctest do SDK KDE 6.11 omite ", 0 tests failed" quando não há falha: "100% tests passed out of N" é zero falha.
# Qualquer outro resumo sem a contagem de falhas continua sendo falha (padrão 1, fechado).
if [ -z "$falharam" ] && printf '%s\n' "$resumo" | grep -q -E '^100% tests passed out of [0-9]+$'; then
    falharam=0
fi
[ -n "$falharam" ] || falharam=1

echo "suite_flatpak: analisados=$analisados falharam=$falharam ctest_rc=$rc"
cat "$BUILD/suite-executados.txt" | tail -n 3
# Diagnóstico (CI, run 41176856725): o ctest guarda a saída do teste que falhou só em $BUILD/suite-executados.txt, e o
# log do job mostra apenas o tail. Quando algum teste falha, as linhas de reprovação dos portões e dos testes vão ao log.
if [ "$rc" -ne 0 ] || [ "$falharam" -ne 0 ]; then
    echo "suite_flatpak: linhas de reprovação dos testes que falharam:"
    grep -E 'VIOLACAO|violacoes=|encontrados=|Traceback|AssertionError|FAIL!|Error|No such|not a git' "$BUILD/suite-executados.txt" | head -n 60
fi

if [ "$analisados" -ne "$encontrados" ]; then
    echo "PARADA: varredura incompleta: executou $analisados de $encontrados testes listados"
    exit 1
fi
if [ "$rc" -ne 0 ] || [ "$falharam" -ne 0 ]; then
    echo "PARADA: $falharam teste(s) falharam (ctest rc=$rc)"
    exit 1
fi
echo "suite_flatpak: VERDE, $analisados de $encontrados testes passaram, piso $piso"
exit 0
