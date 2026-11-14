#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Suíte de unidade do build Flatpak (NL-2.11, DA-2.60 item 1; L-36). Roda, no sandbox do SDK, todos os testes que o CMake
# registra com NL_FLATPAK_BUILD=ON. Exclusão por teste (NL-2.15, DA-2.65): cada guarda do CMake desliga só os slots que usam
# xmlsec1, xmllint ou o servidor falso com oráculo; os que ficam fora estão em flatpak/testes_fora_do_flatpak.txt, com o motivo
# de cada um, e este script confere que nenhum deles aparece registrado.
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

# NL-2.15 (DA-2.65): os testes que o CMake deixa fora do Flatpak estão em flatpak/testes_fora_do_flatpak.txt, com motivo.
# Nenhum deles pode aparecer registrado no _build do Flatpak: se aparecer, a lista está desatualizada e o job reprova.
# Mesma leitura do portão nativo (tools/portoes/fora_do_flatpak.sh): nomes do JSON do próprio CTest, nível de teste.
LISTA_FORA="flatpak/testes_fora_do_flatpak.txt"
ctest --test-dir "$BUILD" --show-only=json-v1 > "$BUILD/suite-json.txt" 2>&1
json_rc=$?
sed -n 's/^      "name" : "\(.*\)",\{0,1\}$/\1/p' "$BUILD/suite-json.txt" | LC_ALL=C sort -u > "$BUILD/suite-registrados.set"
sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' -e 's/[[:space:]]*|.*$//' "$LISTA_FORA" | LC_ALL=C sort -u > "$BUILD/suite-fora.set"
registrados="$(wc -l < "$BUILD/suite-registrados.set" | tr -d ' ')"
fora_listados="$(wc -l < "$BUILD/suite-fora.set" | tr -d ' ')"
registrados_fora="$(LC_ALL=C comm -12 "$BUILD/suite-fora.set" "$BUILD/suite-registrados.set" | wc -l | tr -d ' ')"
echo "suite_flatpak: registrados=$registrados fora_listados=$fora_listados registrados_e_listados=$registrados_fora"
if [ "$json_rc" -ne 0 ] || [ "$registrados" -eq 0 ]; then
    echo "PARADA: ctest --show-only=json-v1 do _build do Flatpak falhou ou não listou testes (NL-2.15)"
    exit 1
fi
if [ "$fora_listados" -eq 0 ]; then
    echo "PARADA: lista $LISTA_FORA sem nenhum teste (NL-2.15): zero excluídos não é varredura"
    exit 1
fi
if [ "$registrados_fora" -ne 0 ]; then
    echo "PARADA: $registrados_fora teste(s) listados em $LISTA_FORA estão registrados no Flatpak (NL-2.15, DA-2.65)"
    exit 1
fi

echo "suite_flatpak: piso=$piso (base $piso_base + mais $mais) encontrados=$encontrados"
if [ "$encontrados" -ne "$piso" ]; then
    if [ "$encontrados" -lt "$piso" ]; then sentido="abaixo do piso"; else sentido="acima do piso"; fi
    echo "PARADA: o CTest registrou $encontrados testes, $sentido $piso (teste perdido ou não registrado; teste novo pede atualizar $PISO_ARQ)"
    exit 1
fi

ctest --test-dir "$BUILD" -j 4 --output-on-failure --no-tests=error > "$BUILD/suite-executados.txt" 2>&1
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
    grep -E 'VIOLACAO|violacoes=|encontrados=|Traceback|AssertionError|FAIL!|Error|No such|not a git|\((Failed|Exception|Timeout)\)' "$BUILD/suite-executados.txt" | head -n 60
fi

if [ "$analisados" -ne "$encontrados" ]; then
    echo "PARADA: varredura incompleta: executou $analisados de $encontrados testes listados"
    exit 1
fi
if [ "$rc" -ne 0 ] || [ "$falharam" -ne 0 ]; then
    echo "PARADA: $falharam teste(s) falharam (ctest rc=$rc)"
    exit 1
fi
# NL-2.14 (DA-2.61): o SDK não mostra pulo no resumo do ctest (QSKIP conta como passou). O pulo sai do junitxml por função,
# comparado por igualdade exata com a lista da distro "flatpak" (vazia). Reprova se houver pulo fora da lista ou linha morta.
if ! sh tools/portoes/pulos_testes.sh "$BUILD" flatpak tests/pulos_esperados_por_distro.txt; then
    echo "PARADA: pulos de teste fora da lista de pulos esperados (NL-2.14, DA-2.61)"
    exit 1
fi
echo "suite_flatpak: VERDE, $analisados de $encontrados testes passaram, piso $piso"
exit 0
