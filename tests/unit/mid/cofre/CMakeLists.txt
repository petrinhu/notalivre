# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do adaptador de cofre do A1 (NL-2.2, DA-2.30): EKU ausente registra evento; falha do registro recusa.
# Um teste no CTest por slot (mid.cofre.<nome>), sem vírgula no nome (L-45). Fixtures da NL-2.1.
find_package(OpenSSL 3.0 REQUIRED)

add_executable(tst_adaptador_credencial tst_adaptador_credencial.cpp)
target_link_libraries(tst_adaptador_credencial PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_pkcs12
                      nl_foundation_log nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")

foreach(nome IN ITEMS
    credencialSemEkuRegistraEventoNoLog
    credencialComEkuNaoRegistraEvento
    falhaDoRegistroRecusaCredencialSemEku)
  add_test(NAME mid.cofre.${nome} COMMAND tst_adaptador_credencial ${nome})
  set_tests_properties(mid.cofre.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
