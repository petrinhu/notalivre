# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Arnês ponta a ponta de `notalivre dps validar` (NL-1.7): roda a CLI em processo próprio e confere saída, XML e erros.
# Oráculo independente: xmllint --schema sobre os XML que a CLI gravou (oraculo_xmllint_dps_validar, depois do arnês).
# A CLI é o alvo do arnês: só o binário é exercitado, nenhuma fachada entra no QtTest.
add_executable(tst_dps_validar tst_dps_validar.cpp)
add_dependencies(tst_dps_validar notalivre-cli)
target_link_libraries(tst_dps_validar PRIVATE Qt6::Core Qt6::Test nl_warnings)
target_compile_definitions(tst_dps_validar PRIVATE
    "NL_CLI_DPS=\"$<TARGET_FILE:notalivre-cli>\""
    "NL_ENTRADAS_DPS=\"${CMAKE_CURRENT_SOURCE_DIR}/entradas\""
    "NL_SAIDA_DPS=\"${CMAKE_BINARY_DIR}/e2e_dps_validar/saida\""
    "NL_FIXTURES_DPS=\"${CMAKE_SOURCE_DIR}/tests/fixtures/dps\""
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\"")
add_test(NAME tst_dps_validar COMMAND tst_dps_validar)

# Oráculo: o mesmo script do validador da NL-1.6a (régua por nome: valida_* deve validar). Lê só a saída do arnês.
find_program(NL_XMLLINT_E2E xmllint REQUIRED)
add_test(NAME oraculo_xmllint_dps_validar
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/unit/foundation/xml/oraculo_xmllint.sh"
        "${NL_XMLLINT_E2E}" "${NL_XSD_PRODREST_DIR}" "${CMAKE_BINARY_DIR}/e2e_dps_validar/saida")
set_tests_properties(oraculo_xmllint_dps_validar PROPERTIES DEPENDS tst_dps_validar)
