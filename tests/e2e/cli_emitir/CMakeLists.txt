# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# E2E de `notalivre emitir` contra o servidor falso (NL-2.9, DA-2.44, DA-2.45). Só fixtures sintéticas. Só em contêiner.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

find_program(NL_XMLLINT_EMITIR xmllint REQUIRED)
find_program(NL_XMLSEC1_EMITIR xmlsec1 REQUIRED)

add_executable(tst_cli_emitir tst_cli_emitir.cpp)
add_dependencies(tst_cli_emitir notalivre-cli)
target_link_libraries(tst_cli_emitir PRIVATE Qt6::Core Qt6::Test util nl_warnings)
target_compile_definitions(tst_cli_emitir PRIVATE
    "NL_CLI_EMITIR=\"$<TARGET_FILE:notalivre-cli>\""
    "NL_LANCADOR_EMITIR=\"${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py\""
    "NL_PYTHON3_EMITIR=\"${Python3_EXECUTABLE}\""
    "NL_CERTIFICADOS_EMITIR=\"${CMAKE_BINARY_DIR}/certificados_teste\""
    "NL_ENTRADAS_EMITIR=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas\""
    "NL_XSD_EMITIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_MANIFESTO_EMITIR=\"${NL_XSD_PRODREST_MANIFESTO}\"")
add_test(NAME e2e.cliEmitir.cliEmiteDpsAceitaPeloServidorFalso COMMAND tst_cli_emitir cliEmiteDpsAceitaPeloServidorFalso)
set_tests_properties(e2e.cliEmitir.cliEmiteDpsAceitaPeloServidorFalso PROPERTIES
                     FIXTURES_REQUIRED certificados_teste
                     ENVIRONMENT "NL_XMLLINT=${NL_XMLLINT_EMITIR};NL_XMLSEC1=${NL_XMLSEC1_EMITIR}"
                     PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                     FAIL_REGULAR_EXPRESSION "FAIL!")
