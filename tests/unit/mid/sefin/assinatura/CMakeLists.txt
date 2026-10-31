# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do adaptador de assinatura da DPS (NL-2.9, DA-2.42). Um slot por CTest, sem vírgula no nome (L-45).
find_package(LibXml2 REQUIRED)

add_executable(tst_assinatura_dps tst_assinatura_dps.cpp)
target_include_directories(tst_assinatura_dps PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries(tst_assinatura_dps PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_pkcs12
                      nl_foundation_xmldsig ${LIBXML2_LIBRARY} nl_warnings nl_teste_apoio)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
add_test(NAME mid.sefin.assinatura.parametrosDaAssinaturaVemDaPastaDoLeiaute
         COMMAND tst_assinatura_dps parametrosDaAssinaturaVemDaPastaDoLeiaute)
add_test(NAME mid.sefin.assinatura.tabelaCobreRuntimeKdeFlatpak
         COMMAND tst_assinatura_dps tabelaCobreRuntimeKdeFlatpak)
set_tests_properties(mid.sefin.assinatura.tabelaCobreRuntimeKdeFlatpak PROPERTIES
                    ENVIRONMENT "NL_TABELA_SHA1_ASSINATURA=${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig/tabela_sha1_por_distro.txt"
                    PASS_REGULAR_EXPRESSION "Totals: [0-9]+ passed, 0 failed, 0 skipped"
                    FAIL_REGULAR_EXPRESSION "FAIL!")
add_test(NAME mid.sefin.assinatura.digestDeComparacaoEhSha256
         COMMAND tst_assinatura_dps digestDeComparacaoEhSha256)
set_tests_properties(mid.sefin.assinatura.digestDeComparacaoEhSha256 PROPERTIES PASS_REGULAR_EXPRESSION "Totals: [0-9]+ passed, 0 failed, 0 skipped" FAIL_REGULAR_EXPRESSION "FAIL!")
add_test(NAME mid.sefin.assinatura.emissaoLigaAdaptadorRealPelaRaiz
         COMMAND tst_assinatura_dps emissaoLigaAdaptadorRealPelaRaiz)
set_tests_properties(mid.sefin.assinatura.emissaoLigaAdaptadorRealPelaRaiz PROPERTIES
                    FIXTURES_REQUIRED certificados_teste
                    ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_FIXTURES_DPS=${CMAKE_SOURCE_DIR}/tests/fixtures/dps;NL_TABELA_SHA1_ASSINATURA=${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig/tabela_sha1_por_distro.txt"
                    PASS_REGULAR_EXPRESSION "Totals: [0-9]+ passed, 0 failed, 0 skipped"
                    FAIL_REGULAR_EXPRESSION "FAIL!")
set_tests_properties(mid.sefin.assinatura.parametrosDaAssinaturaVemDaPastaDoLeiaute PROPERTIES
                    FIXTURES_REQUIRED certificados_teste
                    ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_FIXTURES_DPS=${CMAKE_SOURCE_DIR}/tests/fixtures/dps;NL_TABELA_SHA1_ASSINATURA=${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig/tabela_sha1_por_distro.txt"
                    PASS_REGULAR_EXPRESSION "Totals: [0-9]+ passed, 0 failed, 0 skipped"
                    FAIL_REGULAR_EXPRESSION "FAIL!")
