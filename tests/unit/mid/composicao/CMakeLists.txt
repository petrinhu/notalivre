# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da política de ambiente da emissão e do manipulador do Qt (NL-2.9). Slots sem vírgula (L-45).
add_executable(tst_politica_emissao tst_politica_emissao.cpp)
target_link_libraries(tst_politica_emissao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.politicaEmissao COMMAND tst_politica_emissao)
set_tests_properties(mid.composicao.politicaEmissao PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_manipulador_qt tst_manipulador_qt.cpp)
target_link_libraries(tst_manipulador_qt PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.manipuladorQt COMMAND tst_manipulador_qt)
set_tests_properties(mid.composicao.manipuladorQt PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!"
                     ENVIRONMENT "NL_LOGS_PII_CLI_DIR=${CMAKE_BINARY_DIR}/logs_pii_cli" DEPENDS cli.limpaLogsPii)

add_executable(tst_confirmacao_inscricao tst_confirmacao_inscricao.cpp)
target_link_libraries(tst_confirmacao_inscricao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12 nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.confirmacaoInscricao COMMAND tst_confirmacao_inscricao)
set_tests_properties(mid.composicao.confirmacaoInscricao PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_registro_cli tst_registro_cli.cpp)
target_link_libraries(tst_registro_cli PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.registroCli COMMAND tst_registro_cli)

# Chave do A1 na configuração TLS da emissão (NL-2.10e, E3, H1). Fixture de teste, nunca o .pfx do líder; o handshake usa o servidor
# falso mTLS (tools/servidor_falso/lancador.py).
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)
add_executable(tst_chave_a1 tst_chave_a1.cpp)
target_link_libraries(tst_chave_a1 PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_mid nl_foundation_pkcs12 nl_foundation_transporte nl_warnings)
target_compile_definitions(tst_chave_a1 PRIVATE
    NL_LANCADOR="${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py"
    NL_PYTHON3="${Python3_EXECUTABLE}")
set(NL_CERT_DIR_CHAVE "${CMAKE_BINARY_DIR}/certificados_teste")
foreach(nome IN ITEMS chaveDaFixtureNaoSaiNula handshakeMtlsConcluiComAChaveDeProducao)
  nl_registra_teste_qt(NAME mid.composicao.chaveA1.${nome} COMMAND tst_chave_a1 ${nome})
  set_tests_properties(mid.composicao.chaveA1.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR_CHAVE}"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
set_tests_properties(mid.composicao.registroCli PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

# Revalidação no XSD depois de assinar (NL-2.9, DA-2.51). Só fixtures sintéticas; o XSD vem do build (NL_XSD_PRODREST_DIR).
add_executable(tst_validar_xml tst_validar_xml.cpp)
target_compile_definitions(tst_validar_xml PRIVATE
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_ENTRADA_MEI=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas/mei_minima.json\"")
target_link_libraries(tst_validar_xml PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.validarXmlAceitaDpsValida COMMAND tst_validar_xml validarXmlAceitaDpsValida)
set_tests_properties(mid.composicao.validarXmlAceitaDpsValida PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
nl_registra_teste_qt(NAME mid.composicao.validarXmlRecusaXmlQueNaoConfereNoXsd COMMAND tst_validar_xml validarXmlRecusaXmlQueNaoConfereNoXsd)
set_tests_properties(mid.composicao.validarXmlRecusaXmlQueNaoConfereNoXsd PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

add_executable(tst_codigo_saida tst_codigo_saida.cpp)
target_link_libraries(tst_codigo_saida PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
foreach(nome IN ITEMS codigosDeValorDaDpsSaemComQuatro codigosInternosDeValorSaemComSetenta codigosDeDocumentoSaemComQuatro codigosDeAmbienteXsdSaemComDois codigosDeRecusaSaemComUm dpsDuplicadaSaiComCinco comparacaoIndisponivelSaiComSete codigoDesconhecidoSaiComSetenta configuracaoSaiComDez codigosDoSegredoSaemComDois headlessSemAutorizacaoSaiComOnze)
    nl_registra_teste_qt(NAME mid.composicao.codigoSaida.${nome} COMMAND tst_codigo_saida ${nome})
    set_tests_properties(mid.composicao.codigoSaida.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Revalidação do XML depois de assinar, com assinador falso pela fábrica da fachada (NL-2.9, DA-2.56). Fixtures sintéticas.
add_executable(tst_emissao_revalida tst_emissao_revalida.cpp)
target_compile_definitions(tst_emissao_revalida PRIVATE
    "NL_CERTIFICADOS_EMISSAO=\"${CMAKE_BINARY_DIR}/certificados_teste\""
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_ENTRADA_MEI=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas/mei_minima.json\"")
target_link_libraries(tst_emissao_revalida PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation nl_foundation_pkcs12 nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.emissaoRevalida.xmlInvalidoAposAssinarSaiComXsdPosAssinatura COMMAND tst_emissao_revalida xmlInvalidoAposAssinarSaiComXsdPosAssinatura)
set_tests_properties(mid.composicao.emissaoRevalida.xmlInvalidoAposAssinarSaiComXsdPosAssinatura PROPERTIES
                     FIXTURES_REQUIRED certificados_teste
                     FAIL_REGULAR_EXPRESSION "FAIL!")

# Resposta crua para análise (NL-2.9, DA-2.59 item 4): mascaragem e gravação 0600. Sem fixture real.
add_executable(tst_resposta_crua tst_resposta_crua.cpp)
target_link_libraries(tst_resposta_crua PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.respostaCrua COMMAND tst_resposta_crua)
set_tests_properties(mid.composicao.respostaCrua PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

# Âncoras da emissão (NL-2.10b, emendas 4 e 5): o A1 fora de Homologação usa as raízes ICP do bundle; o servidor TLS, sem PEM,
# usa as CAs do sistema do runtime. A raiz ICP vem do CMake de third_party (NL_ICP_RAIZES_DIR).
add_executable(tst_ancoras_emissao tst_ancoras_emissao.cpp)
target_compile_definitions(tst_ancoras_emissao PRIVATE "NL_ICP_RAIZES_DIR=\"${NL_ICP_RAIZES_DIR}\"")
target_link_libraries(tst_ancoras_emissao PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_mid nl_foundation_pkcs12 nl_foundation_base nl_foundation_log nl_warnings)
foreach(nome IN ITEMS ancorasPadraoSaoAsRaizesIcp ancorasTlsPadraoDoSistema pedidoSemAmbienteUsaODerivadoDoHost ambienteDivergenteDoHostRecusadoNaMid diretorioDeRaizesVazioEErroDeConfiguracao homologacaoUsaOPemDeAncoras hostDesconhecidoEErroNasFuncoesPublicas)
    nl_registra_teste_qt(NAME mid.composicao.ancorasEmissao.${nome} COMMAND tst_ancoras_emissao ${nome})
    set_tests_properties(mid.composicao.ancorasEmissao.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Manifesto do XSD divergente do pin embutido (NL-2.10b, BAIXA-4): a revalidação recusa antes de abrir o esquema.
nl_registra_teste_qt(NAME mid.composicao.validarXmlRecusaManifestoDivergente COMMAND tst_validar_xml validarXmlRecusaManifestoDivergente)
set_tests_properties(mid.composicao.validarXmlRecusaManifestoDivergente PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")

# Leitor do totTrib (NL-2.10j, J1-r2; B-1 da revisão QA): uma forma por grupo, duas formas recusadas, percentual mal formado.
add_executable(tst_ler_tottrib tst_ler_tottrib.cpp)
target_include_directories(tst_ler_tottrib PRIVATE ${CMAKE_SOURCE_DIR}/src/mid/composicao)
target_link_libraries(tst_ler_tottrib PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.lerTotTrib COMMAND tst_ler_tottrib)

# Leitor da inscrição da CLI (NL-2.10h, H4): CNPJ e CPF com zero à esquerda como texto; número JSON recusado.
add_executable(tst_leitor_inscricao tst_leitor_inscricao.cpp)
target_include_directories(tst_leitor_inscricao PRIVATE ${CMAKE_SOURCE_DIR}/src/mid/composicao)
target_link_libraries(tst_leitor_inscricao PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME mid.composicao.leitorInscricao COMMAND tst_leitor_inscricao)
set_tests_properties(mid.composicao.leitorInscricao PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
