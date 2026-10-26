# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários do Id da DPS, série, número e chave de acesso mínima (NL-1.3). Regra pura, sem I/O.
# Um executável por tipo, cada um no CTest pelo próprio nome.
add_executable(tst_serie tst_serie.cpp)
target_link_libraries(tst_serie PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_serie COMMAND tst_serie)

add_executable(tst_numero_dps tst_numero_dps.cpp)
target_link_libraries(tst_numero_dps PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_numero_dps COMMAND tst_numero_dps)

add_executable(tst_chave_acesso tst_chave_acesso.cpp)
target_link_libraries(tst_chave_acesso PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_chave_acesso COMMAND tst_chave_acesso)

add_executable(tst_id_dps tst_id_dps.cpp)
target_link_libraries(tst_id_dps PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_id_dps COMMAND tst_id_dps)
