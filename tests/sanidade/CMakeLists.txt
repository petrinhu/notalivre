# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Prova que o executor de testes (CTest + QtTest) está ligado (plano NL-0.2).
add_executable(tst_sanidade tst_sanidade.cpp)
target_link_libraries(tst_sanidade PRIVATE Qt6::Test nl_foundation_base nl_warnings)
add_test(NAME tst_sanidade COMMAND tst_sanidade)
