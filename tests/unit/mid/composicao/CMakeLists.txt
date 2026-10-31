# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da política de ambiente da emissão e do manipulador do Qt (NL-2.9). Slots sem vírgula (L-45).
add_executable(tst_politica_emissao tst_politica_emissao.cpp)
target_link_libraries(tst_politica_emissao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.politicaEmissao COMMAND tst_politica_emissao)
set_tests_properties(mid.composicao.politicaEmissao PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 6 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_manipulador_qt tst_manipulador_qt.cpp)
target_link_libraries(tst_manipulador_qt PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_warnings)
add_test(NAME mid.composicao.manipuladorQt COMMAND tst_manipulador_qt)
set_tests_properties(mid.composicao.manipuladorQt PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_confirmacao_inscricao tst_confirmacao_inscricao.cpp)
target_link_libraries(tst_confirmacao_inscricao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12 nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.confirmacaoInscricao COMMAND tst_confirmacao_inscricao)
set_tests_properties(mid.composicao.confirmacaoInscricao PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_registro_cli tst_registro_cli.cpp ${CMAKE_SOURCE_DIR}/src/mid/composicao/registro_cli.cpp)
target_link_libraries(tst_registro_cli PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.registroCli COMMAND tst_registro_cli)
set_tests_properties(mid.composicao.registroCli PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 2 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")
