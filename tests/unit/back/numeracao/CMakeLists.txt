# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários da numeração da DPS (NL-1.3): Numerador e motivo de pulo. Regra pura; o repositório é um falso em memória.
add_executable(tst_motivo_pulo tst_motivo_pulo.cpp)
target_link_libraries(tst_motivo_pulo PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_motivo_pulo COMMAND tst_motivo_pulo)

add_executable(tst_numerador tst_numerador.cpp)
target_link_libraries(tst_numerador PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_numerador COMMAND tst_numerador)
