# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do adaptador de cofre do A1 (NL-2.2, DA-2.30) e da conversão para o assinador (NL-2.3, DA-2.36).
# Um teste no CTest por slot (mid.cofre.<nome>), sem vírgula no nome (L-45). Fixtures da NL-2.1.
find_package(OpenSSL 3.0 REQUIRED)

add_executable(tst_adaptador_credencial tst_adaptador_credencial.cpp)
target_link_libraries(tst_adaptador_credencial PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12
                      nl_foundation_log nl_warnings)

add_executable(tst_material_assinatura tst_material_assinatura.cpp)
target_link_libraries(tst_material_assinatura PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12
                      nl_foundation_xmldsig nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")

foreach(nome IN ITEMS
    credencialSemEkuRegistraEventoNoLog
    credencialComEkuNaoRegistraEvento
    falhaDoRegistroRecusaCredencialSemEku)
  nl_registra_teste_qt(NAME mid.cofre.${nome} COMMAND tst_adaptador_credencial ${nome})
  set_tests_properties(mid.cofre.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

foreach(nome IN ITEMS materialDaCredencialViva credencialDescartadaNaoEntregaMaterial)
  nl_registra_teste_qt(NAME mid.cofre.${nome} COMMAND tst_material_assinatura ${nome})
  set_tests_properties(mid.cofre.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Âncoras de produção do A1 (NL-2.13, DA-2.48, CISO 20:20:03 item 1): o conjunto de produção, fixado no binário.
# O SHA-256 do SHA256SUMS embutido vem do CMake de third_party (NL_ICP_RAIZES_MANIFESTO_SHA256), o mesmo que o nl_mid usa.
add_executable(tst_raizes_producao tst_raizes_producao.cpp)
target_link_libraries(tst_raizes_producao PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12
                      OpenSSL::Crypto nl_warnings)
target_compile_definitions(tst_raizes_producao PRIVATE
                           "NL_ICP_RAIZES_MANIFESTO_SHA256=\"${NL_ICP_RAIZES_MANIFESTO_SHA256}\"")

foreach(nome IN ITEMS
    ancorasPadraoDeProducaoSaoAsRaizesIcp
    manifestoComHashDiferenteDoEmbutidoRecusado)
  nl_registra_teste_qt(NAME mid.cofre.${nome} COMMAND tst_raizes_producao ${nome})
  set_tests_properties(mid.cofre.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_ICP_RAIZES_DIR=${NL_ICP_RAIZES_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
