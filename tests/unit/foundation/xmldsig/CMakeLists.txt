# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do assinador XMLDSIG (NL-2.3). Um slot por CTest (xmldsig.<slot>), sem vírgula no nome (L-45), com a linha
# "Totals: 1 passed, 0 failed" do QTest. Os slots que pedem xmlsec1 (ferramenta de teste, DA-2.1) são desligados no
# build Flatpak (NL_FLATPAK_BUILD, DA-2.28): o SDK não traz a ferramenta, e isso aparece no log do configure.
# O slot assinaSha1NoSdkKdeSemVariavel só existe no Flatpak: é a prova de que o SDK assina sem a variável.
find_package(OpenSSL 3.0 REQUIRED)
find_package(LibXml2 REQUIRED)

add_executable(tst_assinador_xml tst_assinador_xml.cpp)
target_include_directories(tst_assinador_xml PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries(tst_assinador_xml PRIVATE Qt6::Core Qt6::Test nl_foundation_xmldsig nl_foundation_pkcs12
                      ${LIBXML2_LIBRARY} OpenSSL::Crypto nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
set(NL_AMBIENTE_XMLDSIG
    "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_FIXTURES_DPS=${CMAKE_SOURCE_DIR}/tests/fixtures/dps;NL_FIXTURES_XMLDSIG=${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig;NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR}")

if(NL_FLATPAK_BUILD)
    message(STATUS "xmldsig: slots com xmlsec1 desligados no build Flatpak (NL_FLATPAK_BUILD=ON, DA-2.28)")
    set(NL_SLOTS_XMLDSIG assinaSha1NoSdkKdeSemVariavel)
else()
    find_program(NL_XMLLINT_XMLDSIG xmllint REQUIRED)
    find_program(NL_XMLSEC1_XMLDSIG xmlsec1 REQUIRED)
    set(NL_AMBIENTE_XMLDSIG "${NL_AMBIENTE_XMLDSIG};NL_XMLLINT=${NL_XMLLINT_XMLDSIG};NL_XMLSEC1=${NL_XMLSEC1_XMLDSIG}")
    set(NL_SLOTS_XMLDSIG
        assinaturaConfereNoXmlsec1Cli
        xmlAlteradoDepoisDeAssinarReprova
        xmlAssinadoContinuaValidoNoXsd
        signatureIrmaDeInfDps
        referenciaApontaParaIdDoInfDps
        keyInfoSoComX509Certificate
        namespaceHerdadoEntraNaCanonicalizacao
        algoritmoVemDaPastaDoLeiaute
        digestValueIgualAoDouradoEmTodasAsImagens
        digestSha256IgualAoDouradoEmTodasAsImagens
        assinaturaVerificaComAChavePublicaDaFixture
        algoritmoBloqueadoViraErroProprio
        materialNuloDevolveErroProprio
        materialEmprestadoNaoLiberaNemCopia
        algoritmoDaTabelaPorDistro
        medirOsslLibCtxProprio)
endif()

foreach(nome IN LISTS NL_SLOTS_XMLDSIG)
    add_test(NAME xmldsig.${nome} COMMAND tst_assinador_xml ${nome})
    set_tests_properties(xmldsig.${nome} PROPERTIES
                         FIXTURES_REQUIRED certificados_teste
                         ENVIRONMENT "${NL_AMBIENTE_XMLDSIG}"
                         PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                         FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
