# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários da Foundation base (NL-1.12): tipo Erro. Regra pura, sem I/O.
add_executable(tst_erro tst_erro.cpp)
target_link_libraries(tst_erro PRIVATE Qt6::Test nl_foundation_base nl_warnings)
add_test(NAME tst_erro COMMAND tst_erro)
