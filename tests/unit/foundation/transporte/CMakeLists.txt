# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de integração do transporte HTTPS com mTLS (NL-2.5). Um teste no CTest por slot (transporte.<nome>), sem
# vírgula no nome (L-45). Cada um depende da fixture de certificados (NL-2.1) e do servidor falso (NL-2.6), que o
# próprio teste sobe como processo Python (tools/servidor_falso/lancador.py).
find_package(OpenSSL 3.0 REQUIRED)
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

add_executable(tst_transporte_https tst_transporte_https.cpp)
target_link_libraries(tst_transporte_https PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_foundation_transporte
                      OpenSSL::Crypto nl_warnings)
target_compile_definitions(tst_transporte_https PRIVATE
    NL_LANCADOR="${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py"
    NL_PYTHON3="${Python3_EXECUTABLE}")

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")

foreach(nome IN ITEMS
    semCertificadoDeClienteHandshakeRecusado
    certificadoDeOutraAcRecusado
    timeoutViraErroTransitorio
    baseEPrefixoVemDaConfiguracao
    conexaoRecusadaViraErroTransitorio
    corpo2xxTruncadoViraTransitorio
    rejeicao4xxComCorpoContinuaRespostaHttp
    cabecalhoAcceptEncodingIdentityEnviado
    contentEncodingNaoIdentityViraDefinitivoProtocolo
    mensagemDeErroNaoTrazURLNemHost)
  add_test(NAME transporte.${nome} COMMAND tst_transporte_https ${nome})
  set_tests_properties(transporte.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
