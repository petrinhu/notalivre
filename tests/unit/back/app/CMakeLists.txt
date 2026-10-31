# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do caso de uso EmitirNota (NL-2.9, DA-2.42), com o port em memória. Slots sem vírgula (L-45).
add_executable(tst_emitir_nota tst_emitir_nota.cpp)
target_link_libraries(tst_emitir_nota PRIVATE Qt6::Core Qt6::Test nl_back nl_foundation_base nl_warnings)
add_test(NAME back.app.emitirNota COMMAND tst_emitir_nota)
set_tests_properties(back.app.emitirNota PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 5 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")
