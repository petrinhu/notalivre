# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do carregador de A1 (NL-2.2, ADR-S1). Um teste no CTest por slot (pkcs12.<nome>), sem vírgula no nome (L-45).
# Cada um depende da fixture de certificados (NL-2.1) e passa só com a linha "Totals: 1 passed, 0 failed" do QTest.
find_package(OpenSSL 3.0 REQUIRED)

add_executable(tst_carregador_pkcs12 tst_carregador_pkcs12.cpp)
target_link_libraries(tst_carregador_pkcs12 PRIVATE Qt6::Core Qt6::Test nl_foundation_pkcs12 nl_foundation_log
                      OpenSSL::Crypto nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")

foreach(nome IN ITEMS
    senhaCorretaCarrega
    senhaErradaDaErroProprio
    senhaErradaNoLegadoDaErroProprio
    cifraLegadaCarregaComProviderLegado
    legadoNaoDesligaODefault
    vencidoSinalizado
    semEkuClientAuthSinalizado
    ekuAusenteAceitaComEventoSemPii
    cnpjExtraidoDoCertificado
    cnpjAlfanumericoExtraidoComoTexto
    cpfExtraidoDoCertificado
    raizComparadaComoTexto
    inscricaoFormatoInvalidoRecusado
    semInscricaoSinalizada
    cadeiaDeOutraRaizReprova
    cadeiaIncompletaReprova
    cadeiaComSubordinadaCarrega
    pfxMalformadoReprova
    ancorasDePemLemUmCertificado
    ancorasSemCertificadoReprovam
    senhaSeguraZeraConteudo
    descartarLiberaChave)
  add_test(NAME pkcs12.${nome} COMMAND tst_carregador_pkcs12 ${nome})
  set_tests_properties(pkcs12.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
