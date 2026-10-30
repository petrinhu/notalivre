# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da linha de comando de `notalivre emitir` e da exceção do topo (NL-2.9). Slots sem vírgula (L-45).
# Os fontes da CLI entram direto nos testes, porque o executável notalivre-cli não é biblioteca.
set(NL_FONTES_CLI ${CMAKE_SOURCE_DIR}/apps/cli/argumentos_emitir.cpp ${CMAKE_SOURCE_DIR}/apps/cli/comando_emitir.cpp
    ${CMAKE_SOURCE_DIR}/apps/cli/senha_terminal.cpp ${CMAKE_SOURCE_DIR}/apps/cli/excecao_topo.cpp)

add_executable(tst_argumentos_emitir tst_argumentos_emitir.cpp ${CMAKE_SOURCE_DIR}/apps/cli/argumentos_emitir.cpp)
target_include_directories(tst_argumentos_emitir PRIVATE ${CMAKE_SOURCE_DIR}/apps/cli)
target_link_libraries(tst_argumentos_emitir PRIVATE Qt6::Core Qt6::Test nl_warnings)
add_test(NAME cli.argumentosEmitir COMMAND tst_argumentos_emitir)
set_tests_properties(cli.argumentosEmitir PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 5 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_comando_emitir tst_comando_emitir.cpp ${NL_FONTES_CLI})
target_include_directories(tst_comando_emitir PRIVATE ${CMAKE_SOURCE_DIR}/apps/cli)
target_link_libraries(tst_comando_emitir PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_warnings)
add_test(NAME cli.comandoEmitir COMMAND tst_comando_emitir)
set_tests_properties(cli.comandoEmitir PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 4 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_excecao_topo tst_excecao_topo.cpp ${CMAKE_SOURCE_DIR}/apps/cli/excecao_topo.cpp)
target_include_directories(tst_excecao_topo PRIVATE ${CMAKE_SOURCE_DIR}/apps/cli)
target_link_libraries(tst_excecao_topo PRIVATE Qt6::Core Qt6::Test nl_foundation_log nl_warnings)
add_test(NAME cli.excecaoTopo COMMAND tst_excecao_topo)
set_tests_properties(cli.excecaoTopo PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

# Portão de PII nos logs da CLI (NL-2.9, ADR-0005 e): os testes da CLI gravam os logs deles na pasta própria, e o mesmo
# varredor do Log (tests/unit/foundation/log/varredura_logs.py) confere a pasta com os termos da fixture de referência.
# A limpeza roda antes, para que um log antigo não mascare uma varredura vazia (L-36). Varredura sem .log reprova.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)
set(NL_LOGS_PII_CLI_DIR "${CMAKE_BINARY_DIR}/logs_pii_cli")
add_test(NAME cli.limpaLogsPii COMMAND ${CMAKE_COMMAND} -E rm -rf "${NL_LOGS_PII_CLI_DIR}")
set_tests_properties(cli.excecaoTopo PROPERTIES DEPENDS cli.limpaLogsPii ENVIRONMENT "NL_LOGS_PII_CLI_DIR=${NL_LOGS_PII_CLI_DIR}")
add_test(NAME cli.portaoPiiVarreLogsDaCli
    COMMAND ${Python3_EXECUTABLE} -I "${CMAKE_SOURCE_DIR}/tests/unit/foundation/log/varredura_logs.py"
        --dir "${NL_LOGS_PII_CLI_DIR}" --termos "${CMAKE_SOURCE_DIR}/tests/fixtures/pii/referencia.txt")
set_tests_properties(cli.portaoPiiVarreLogsDaCli PROPERTIES DEPENDS cli.excecaoTopo)
