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
nl_registra_teste_qt(NAME tst_validador_xsd COMMAND tst_validador_xsd)

# Teste de hash do ZIP (mutação M3): configure hermético de um projeto mínimo, com ZIP adulterado e com ZIP certo.
# NL-2.14 (adendo 5): interpretador absoluto (find_program) e caminhos do build pelo ambiente, nunca como argumento.
find_program(NL_BASH_TESTE bash REQUIRED)
add_test(NAME hash_zip_adulterado_recusado
    COMMAND "${NL_BASH_TESTE}" "${CMAKE_CURRENT_SOURCE_DIR}/hash_zip/teste_hash_zip.sh" adulterado
        "${CMAKE_SOURCE_DIR}" "${CMAKE_GENERATOR}")
set_tests_properties(hash_zip_adulterado_recusado PROPERTIES
    ENVIRONMENT "NL_HASH_ZIP_TRABALHO=${CMAKE_BINARY_DIR}/teste_hash_zip/adulterado;NL_XSD_ZIP_CERTO=${NL_XSD_PRODREST_ZIP}")
add_test(NAME hash_zip_controle_aceito
    COMMAND "${NL_BASH_TESTE}" "${CMAKE_CURRENT_SOURCE_DIR}/hash_zip/teste_hash_zip.sh" controle
        "${CMAKE_SOURCE_DIR}" "${CMAKE_GENERATOR}")
set_tests_properties(hash_zip_controle_aceito PROPERTIES
    ENVIRONMENT "NL_HASH_ZIP_TRABALHO=${CMAKE_BINARY_DIR}/teste_hash_zip/controle;NL_XSD_ZIP_CERTO=${NL_XSD_PRODREST_ZIP}")

find_program(NL_XMLLINT xmllint REQUIRED)
add_test(NAME oraculo_xmllint
    COMMAND "${NL_BASH_TESTE}" "${CMAKE_CURRENT_SOURCE_DIR}/oraculo_xmllint.sh" "${NL_XMLLINT}")
set_tests_properties(oraculo_xmllint PROPERTIES
    ENVIRONMENT "NL_XSD_DIR=${NL_XSD_PRODREST_DIR};NL_FIXTURES_ORACULO=${CMAKE_SOURCE_DIR}/tests/fixtures/xsd")
