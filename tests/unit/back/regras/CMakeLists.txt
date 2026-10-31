# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários das regras locais do Back (NL-1.5): uma suíte por regra E, uma por aviso e uma do validador (L-34).
# Regra pura, sem I/O; o catálogo de localidades é falso em memória (catalogo_falso.h, L-33).

add_executable(tst_regra_e0202 tst_regra_e0202.cpp)
target_link_libraries(tst_regra_e0202 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0202 COMMAND tst_regra_e0202)

add_executable(tst_regra_e0204 tst_regra_e0204.cpp)
target_link_libraries(tst_regra_e0204 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0204 COMMAND tst_regra_e0204)

add_executable(tst_regra_e0237 tst_regra_e0237.cpp)
target_link_libraries(tst_regra_e0237 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0237 COMMAND tst_regra_e0237)

add_executable(tst_regra_e0241 tst_regra_e0241.cpp)
target_link_libraries(tst_regra_e0241 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0241 COMMAND tst_regra_e0241)

add_executable(tst_regra_e0246 tst_regra_e0246.cpp)
target_link_libraries(tst_regra_e0246 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0246 COMMAND tst_regra_e0246)

add_executable(tst_regra_e0302 tst_regra_e0302.cpp)
target_link_libraries(tst_regra_e0302 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0302 COMMAND tst_regra_e0302)

add_executable(tst_regra_e0390 tst_regra_e0390.cpp)
target_link_libraries(tst_regra_e0390 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0390 COMMAND tst_regra_e0390)

add_executable(tst_regra_e0532 tst_regra_e0532.cpp)
target_link_libraries(tst_regra_e0532 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0532 COMMAND tst_regra_e0532)

add_executable(tst_regra_e0583 tst_regra_e0583.cpp)
target_link_libraries(tst_regra_e0583 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0583 COMMAND tst_regra_e0583)

add_executable(tst_regra_e0710 tst_regra_e0710.cpp)
target_link_libraries(tst_regra_e0710 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0710 COMMAND tst_regra_e0710)

add_executable(tst_regra_e0850 tst_regra_e0850.cpp)
target_link_libraries(tst_regra_e0850 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0850 COMMAND tst_regra_e0850)

add_executable(tst_regra_e0903 tst_regra_e0903.cpp)
target_link_libraries(tst_regra_e0903 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_regra_e0903 COMMAND tst_regra_e0903)

add_executable(tst_aviso_d1 tst_aviso_d1.cpp)
target_link_libraries(tst_aviso_d1 PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_aviso_d1 COMMAND tst_aviso_d1)

add_executable(tst_validador_local tst_validador_local.cpp)
target_link_libraries(tst_validador_local PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_validador_local COMMAND tst_validador_local)
