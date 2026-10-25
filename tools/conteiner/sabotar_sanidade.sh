#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Sabotagem de estreia do tst_sanidade (NL-0.4, achado A0-05): o build nativo TEM de reprovar uma
# sabotagem do próprio teste, pelo motivo esperado (L-36). Sabota só uma CÓPIA em NL_TMP_BASE (L-27),
# nunca a árvore. Roda no contêiner nativo (Qt 6.12 em QT_PREFIXO ou CMAKE_PREFIX_PATH).
#
# Uso: sabotar_sanidade.sh [RAIZ]     (padrão: raiz do repositório)
# Saída: "sabotagem=tst_sanidade build_rc=B ctest_rc=C motivo=ok|nao" e o veredito.
# Sai com 1 se o teste não reprovar pelo motivo esperado, ou se a sabotagem não chegou ao código.
set -u

RAIZ="${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}"
BASE="${NL_TMP_BASE:-/var/tmp}"
PREFIXO="${CMAKE_PREFIX_PATH:-${QT_PREFIXO:-}}"
[ -n "$PREFIXO" ] || { echo "REPROVA: defina QT_PREFIXO ou CMAKE_PREFIX_PATH (Qt 6.12)"; exit 1; }
TRAB="$BASE/notalivre-sabotagem-sanidade"
TESTE=tests/sanidade/tst_sanidade.cpp

rm -rf -- "$TRAB"
mkdir -p "$TRAB/src" "$TRAB/build" || { echo "REPROVA: sem pasta de trabalho em $TRAB"; exit 1; }
(cd "$RAIZ" && tar --exclude=./.git --exclude='./build*' -cf - .) | tar -C "$TRAB/src" -xf - || { echo "REPROVA: cópia da árvore falhou"; exit 1; }

# Sabotagem: o teste passa a esperar um nome de camada que a base não devolve.
sed -i 's/std::string_view{"base"}/std::string_view{"SABOTAGEM"}/' "$TRAB/src/$TESTE"
chegou=$(grep -c 'SABOTAGEM' "$TRAB/src/$TESTE" || true)
if [ "$chegou" != 1 ]; then
    echo "REPROVA: a sabotagem não chegou ao teste (ocorrências=$chegou)"
    exit 1
fi

cmake -S "$TRAB/src" -B "$TRAB/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$PREFIXO" \
    > "$TRAB/configure.log" 2>&1
cmake --build "$TRAB/build" --target tst_sanidade > "$TRAB/build.log" 2>&1
build_rc=$?
ctest --test-dir "$TRAB/build" -R tst_sanidade --output-on-failure > "$TRAB/ctest.log" 2>&1
ctest_rc=$?

motivo=nao
if [ "$build_rc" -eq 0 ] && [ "$ctest_rc" -ne 0 ] && grep -q 'FAIL!.*nomeCamadaDaBaseConfere' "$TRAB/ctest.log"; then
    motivo=ok
fi
echo "sabotagem=tst_sanidade build_rc=$build_rc ctest_rc=$ctest_rc motivo=$motivo"
if [ "$motivo" = ok ]; then
    echo "VEREDITO: tst_sanidade reprovou a própria sabotagem pelo motivo esperado (log: $TRAB/ctest.log)"
    exit 0
fi
echo "VEREDITO: tst_sanidade NÃO reprovou pelo motivo esperado (verde falso). Log: $TRAB/ctest.log"
tail -n 20 "$TRAB/ctest.log"
exit 1
