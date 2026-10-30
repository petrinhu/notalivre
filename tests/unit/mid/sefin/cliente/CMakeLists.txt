# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do ClienteSefin (NL-2.8): política de retry, catálogo de erros, leitor de erro e cliente contra o servidor
# falso (tools/servidor_falso, pelo lançador com oráculo). Slots mid.sefin.cliente.<nome>, sem vírgula no nome (L-45).
find_package(OpenSSL 3.0 REQUIRED)
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

add_executable(tst_cliente_sefin tst_cliente_sefin.cpp)
target_link_libraries(tst_cliente_sefin PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_mid nl_back nl_foundation_pkcs12
                      nl_foundation_transporte nl_foundation_log OpenSSL::Crypto nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
find_program(NL_XMLLINT_CLIENTE xmllint REQUIRED)
find_program(NL_XMLSEC1_CLIENTE xmlsec1 REQUIRED)
set(NL_AMBIENTE_CLIENTE
    "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_FIXTURES_DPS=${CMAKE_SOURCE_DIR}/tests/fixtures/dps;NL_LANCADOR=${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py;NL_PYTHON3=${Python3_EXECUTABLE};NL_XMLLINT=${NL_XMLLINT_CLIENTE};NL_XMLSEC1=${NL_XMLSEC1_CLIENTE};NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR}")

foreach(nome IN ITEMS
    sucessoExtraiChaveDe50Posicoes
    postTransitorioConsultaDpsAntesDeReenviar
    dps404E2404LiberaReenvio
    conflito409ConsultaENaoReenvia
    rejeicaoE0014NaoRepete
    certificadoRevogadoMapeadoPelaRejeicaoDaSefin
    textoLivreDaApiComCpfSoLogaCodigoE
    getTransitorioRepeteComBackoff
    leitorAceitaErroEErrosECodigoECodigo
    catalogoDaMensagemDoCodigoE
    backoffDobraAteAPrimeiraRepeticao
    xmlNfseGuardaXmlDescomprimido
    base64InvalidoRecusado
    xmlIlegivelNumSucessoNaoLiberaReenvio)
  add_test(NAME mid.sefin.cliente.${nome} COMMAND tst_cliente_sefin ${nome})
  set_tests_properties(mid.sefin.cliente.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "${NL_AMBIENTE_CLIENTE}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
