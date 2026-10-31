#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Checagem estática do registro Qt Test (NL-2.14, revisão devops ALTA-1; L-36). Nenhum add_test de executável tst_ pode ficar
# fora de nl_registra_teste_qt. É reforço: a conferência que vale é registro_qt.sh, que cruza o CTest com o manifesto.
# Uso: registro_qt_estatico.sh <raiz>   (lê <raiz>/tests/**/CMakeLists.txt; sem laço por arquivo)
set -u

RAIZ="${1:?uso: registro_qt_estatico.sh <raiz>}"
ACHADOS=$(find "$RAIZ/tests" -name CMakeLists.txt -exec cat {} + | tr '\n' ' ' | grep -o 'add_test([^)]*)' | grep 'COMMAND tst_' || true)
if [ -n "$ACHADOS" ]; then
    echo "PARADA: add_test de Qt Test fora da função nl_registra_teste_qt (sem junitxml e fora do manifesto):"
    printf '%s\n' "$ACHADOS"
    exit 1
fi
echo "registro_qt_estatico: VERDE, nenhum add_test de tst_ fora da função"
exit 0
