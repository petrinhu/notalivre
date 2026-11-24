# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do valor monetário (NL-1.2): QtTest sob CTest, no padrão de tests/sanidade.
add_executable(tst_valor tst_valor.cpp)
target_link_libraries(tst_valor PRIVATE Qt6::Core Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_valor COMMAND tst_valor)

# Não-compilação do dinheiro (NL-3.1e; decisão do líder, 23/11/2026, "Só o bloqueio no tipo"): Valor::de(10.5) não compila.
# Reusa o script da Foundation sql. Sem nl_warnings: um aviso não pode contar como falha de compilação.
get_target_property(NL_INCLUDES_QT_CORE_DINHEIRO Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
list(FILTER NL_INCLUDES_QT_CORE_DINHEIRO EXCLUDE REGEX "\\$<")
set(NL_INCLUDES_DINHEIRO ${NL_INCLUDES_QT_CORE_DINHEIRO}
    ${CMAKE_SOURCE_DIR}/src/foundation/base/include
    ${CMAKE_SOURCE_DIR}/src/back/include)
string(REPLACE ";" ":" NL_INCLUDES_DINHEIRO_TEXTO "${NL_INCLUDES_DINHEIRO}")
if(NOT NL_FLATPAK_BUILD)
    add_test(NAME dinheiro.naoCompila.valorDeDouble
             COMMAND ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/tests/unit/foundation/sql/nao_compila.cmake")
    set_tests_properties(dinheiro.naoCompila.valorDeDouble PROPERTIES ENVIRONMENT
        "NL_CXX=${CMAKE_CXX_COMPILER};NL_INCLUDES=${NL_INCLUDES_DINHEIRO_TEXTO};NL_NEGATIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/valorDeDouble_negativo.cpp;NL_POSITIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/valorDeDouble_positivo.cpp")
endif()
