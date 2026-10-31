# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da política de ambiente da emissão e do manipulador do Qt (NL-2.9). Slots sem vírgula (L-45).
add_executable(tst_politica_emissao tst_politica_emissao.cpp)
target_link_libraries(tst_politica_emissao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.politicaEmissao COMMAND tst_politica_emissao)
set_tests_properties(mid.composicao.politicaEmissao PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 8 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_manipulador_qt tst_manipulador_qt.cpp)
target_link_libraries(tst_manipulador_qt PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_warnings)
add_test(NAME mid.composicao.manipuladorQt COMMAND tst_manipulador_qt)
set_tests_properties(mid.composicao.manipuladorQt PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 5 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_confirmacao_inscricao tst_confirmacao_inscricao.cpp)
target_link_libraries(tst_confirmacao_inscricao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12 nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.confirmacaoInscricao COMMAND tst_confirmacao_inscricao)
set_tests_properties(mid.composicao.confirmacaoInscricao PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 5 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_registro_cli tst_registro_cli.cpp)
target_link_libraries(tst_registro_cli PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.registroCli COMMAND tst_registro_cli)
set_tests_properties(mid.composicao.registroCli PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 7 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

# Revalidação no XSD depois de assinar (NL-2.9, DA-2.51). Só fixtures sintéticas; o XSD vem do build (NL_XSD_PRODREST_DIR).
add_executable(tst_validar_xml tst_validar_xml.cpp)
target_compile_definitions(tst_validar_xml PRIVATE
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_ENTRADA_MEI=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas/mei_minima.json\"")
target_link_libraries(tst_validar_xml PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.validarXmlAceitaDpsValida COMMAND tst_validar_xml validarXmlAceitaDpsValida)
set_tests_properties(mid.composicao.validarXmlAceitaDpsValida PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")
add_test(NAME mid.composicao.validarXmlRecusaXmlQueNaoConfereNoXsd COMMAND tst_validar_xml validarXmlRecusaXmlQueNaoConfereNoXsd)
set_tests_properties(mid.composicao.validarXmlRecusaXmlQueNaoConfereNoXsd PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_codigo_saida tst_codigo_saida.cpp)
target_link_libraries(tst_codigo_saida PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
foreach(nome IN ITEMS codigosDeDocumentoSaemComQuatro codigosDeAmbienteXsdSaemComDois codigosDeRecusaSaemComUm dpsDuplicadaSaiComCinco comparacaoIndisponivelSaiComSete codigoDesconhecidoSaiComSetenta)
    add_test(NAME mid.composicao.codigoSaida.${nome} COMMAND tst_codigo_saida ${nome})
    set_tests_properties(mid.composicao.codigoSaida.${nome} PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Revalidação do XML depois de assinar, com assinador falso pela fábrica da fachada (NL-2.9, DA-2.56). Fixtures sintéticas.
add_executable(tst_emissao_revalida tst_emissao_revalida.cpp)
target_compile_definitions(tst_emissao_revalida PRIVATE
    "NL_CERTIFICADOS_EMISSAO=\"${CMAKE_BINARY_DIR}/certificados_teste\""
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_ENTRADA_MEI=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas/mei_minima.json\"")
target_link_libraries(tst_emissao_revalida PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation nl_foundation_pkcs12 nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.emissaoRevalida.xmlInvalidoAposAssinarSaiComXsdPosAssinatura COMMAND tst_emissao_revalida xmlInvalidoAposAssinarSaiComXsdPosAssinatura)
set_tests_properties(mid.composicao.emissaoRevalida.xmlInvalidoAposAssinarSaiComXsdPosAssinatura PROPERTIES
                     FIXTURES_REQUIRED certificados_teste
                     PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")

# Resposta crua para análise (NL-2.9, DA-2.59 item 4): mascaragem e gravação 0600. Sem fixture real.
add_executable(tst_resposta_crua tst_resposta_crua.cpp)
target_link_libraries(tst_resposta_crua PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
add_test(NAME mid.composicao.respostaCrua COMMAND tst_resposta_crua)
set_tests_properties(mid.composicao.respostaCrua PROPERTIES PASS_REGULAR_EXPRESSION "Totals: 5 passed, 0 failed" FAIL_REGULAR_EXPRESSION "FAIL!")
