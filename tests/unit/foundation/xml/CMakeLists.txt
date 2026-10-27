# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do validador XSD (NL-1.6a): QtTest sobre as fixtures de tests/fixtures/xsd, e o oráculo xmllint (CTest).
# Os dois leem a mesma pasta e a mesma regra de nome (valida_* deve validar, invalida_* deve falhar).
add_executable(tst_validador_xsd tst_validador_xsd.cpp)
target_link_libraries(tst_validador_xsd PRIVATE Qt6::Test nl_foundation nl_warnings)
target_compile_definitions(tst_validador_xsd PRIVATE
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_XSD_PRODREST_RAIZ=\"${NL_XSD_PRODREST_RAIZ}\""
    "NL_FIXTURES_XSD=\"${CMAKE_SOURCE_DIR}/tests/fixtures/xsd\"")
add_test(NAME tst_validador_xsd COMMAND tst_validador_xsd)

# Teste de hash do ZIP (mutação M3): configure hermético de um projeto mínimo, com ZIP adulterado e com ZIP certo.
add_test(NAME hash_zip_adulterado_recusado
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/hash_zip/teste_hash_zip.sh" adulterado
        "${CMAKE_SOURCE_DIR}" "${NL_XSD_PRODREST_ZIP}" "${CMAKE_BINARY_DIR}/teste_hash_zip/adulterado" "${CMAKE_GENERATOR}")
add_test(NAME hash_zip_controle_aceito
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/hash_zip/teste_hash_zip.sh" controle
        "${CMAKE_SOURCE_DIR}" "${NL_XSD_PRODREST_ZIP}" "${CMAKE_BINARY_DIR}/teste_hash_zip/controle" "${CMAKE_GENERATOR}")

find_program(NL_XMLLINT xmllint REQUIRED)
add_test(NAME oraculo_xmllint
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/oraculo_xmllint.sh"
        "${NL_XMLLINT}" "${NL_XSD_PRODREST_DIR}" "${CMAKE_SOURCE_DIR}/tests/fixtures/xsd")
