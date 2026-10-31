# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do assinador XMLDSIG (NL-2.3). Um slot por CTest (xmldsig.<slot>), sem vírgula no nome (L-45), com a linha
# "Totals: N passed, 0 failed" do QTest, com N = slots + 2 (initTestCase e cleanupTestCase contam como passes; Totals: 1 num slot só, sem o que conta o QTest, nunca aparece). Os slots que pedem xmlsec1 (ferramenta de teste, DA-2.1) são desligados no
# build Flatpak (NL_FLATPAK_BUILD, DA-2.28): o SDK não traz a ferramenta, e isso aparece no log do configure.
# O slot assinaSha1NoSdkKdeSemVariavel só existe no Flatpak: é a prova de que o SDK assina sem a variável.
find_package(OpenSSL 3.0 REQUIRED)
find_package(LibXml2 REQUIRED)

add_executable(tst_assinador_xml tst_assinador_xml.cpp)
target_include_directories(tst_assinador_xml PRIVATE ${LIBXML2_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/foundation/xmldsig)
target_link_libraries(tst_assinador_xml PRIVATE Qt6::Core Qt6::Test nl_foundation_xmldsig nl_foundation_pkcs12
                      ${LIBXML2_LIBRARY} OpenSSL::Crypto nl_warnings nl_teste_apoio)

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
        signedInfoCanonicalizadoNoContextoDoDocumento
        namespacePrefixadoDaRaizEntraNoDigest
        transformadaEnvelopedExcluiASignatureDoDigest
        xmlJaAssinadoRecusado
        idRepetidoRecusado
        idPorPrefixoNaoCasa
        idNaRaizRecusado
        idEmOutroElementoRecusado
        idComAspasNaoInjetaAtributo
        xmlComDtdRecusado
        dtdComSubconjuntoMalformadoRecusadoAntesDoParse
        xmlComDoctypeSystemRemotoNaoAcessaARede
        paradaNoDoctypeNaoLeOSubconjunto
        contextoDoParserBloqueiaRede
        variavelDoFedoraValeNoContextoProprio
        materialVencidoNaHoraDeAssinarRecusado
        materialAindaNaoValidoRecusado
        tabelaPorDistroVemDoArquivo
        atributoXmlDaRaizEntraNaCanonicalizacao
        digestIgualAntesEDepoisDeAssinar
        digestMudaComCampoAlterado
        digestIdRepetidoDaErro
        digestIdAusenteDaErro
        digestDtdDaErro
        digestDaNfseNoFormatoDoXsdIgualAoDaDps
        digestDaNfseAlteradaDifere
        digestNfseIdRepetidoDaErro
        digestInfDpsForaDoCaminhoFixoRecusado
        dtdComExpansaoDeEntidadesRecusadaSemProcessar
        distroRecusaSha1VemDaTabelaNaoDoCodigo
        distroAtualTemLinhaNaTabela)
endif()

# NL-2.14 (adendo 2 da DA-2.61): piso da tabela de HTTP por distro. Roda nos dois builds (a tabela é um arquivo do repositório).
list(APPEND NL_SLOTS_XMLDSIG tabelaHttpTemLinhaQueBloqueiaRede)

# PASS exige zero falha e zero pulo (NL-2.9b, NL-2.14): um QSKIP reprova o slot, mesmo com o resto verde.
foreach(nome IN LISTS NL_SLOTS_XMLDSIG)
    nl_registra_teste_qt(NAME xmldsig.${nome} COMMAND tst_assinador_xml ${nome})
    set_tests_properties(xmldsig.${nome} PROPERTIES
                         FIXTURES_REQUIRED certificados_teste
                         ENVIRONMENT "${NL_AMBIENTE_XMLDSIG}"
                         PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed, 0 skipped"
                         FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# O slot contextoDoParserBloqueiaRede não pula mais e não grava marca em arquivo (NL-2.14, MÉDIA-2): a obrigação de rodar nas
# imagens nativas é de tests/testes_obrigatorios_por_distro.txt, conferida pelo portão de registro.

# DA-2.40 (adendo): o contexto próprio ignora a configuração global. Processo próprio do CTest, com OPENSSL_CONF do arquivo
# de teste. Só no build nativo: o Flatpak não tem o material de teste nem o xmlsec1 (NL_FLATPAK_BUILD, DA-2.28).
if(NOT NL_FLATPAK_BUILD)
    nl_registra_teste_qt(NAME xmldsig.contextoProprioIgnoraConfiguracaoGlobal COMMAND tst_assinador_xml contextoProprioIgnoraConfiguracaoGlobal)
    set_tests_properties(xmldsig.contextoProprioIgnoraConfiguracaoGlobal PROPERTIES
                         FIXTURES_REQUIRED certificados_teste
                         ENVIRONMENT "${NL_AMBIENTE_XMLDSIG};OPENSSL_CONF=${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig/openssl_global_restrito.cnf"
                         PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed, 0 skipped"
                         FAIL_REGULAR_EXPRESSION "FAIL!")
endif()
