# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do valor monetário (NL-1.2): QtTest sob CTest, no padrão de tests/sanidade.
add_executable(tst_valor tst_valor.cpp)
target_link_libraries(tst_valor PRIVATE Qt6::Core Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_valor COMMAND tst_valor)
