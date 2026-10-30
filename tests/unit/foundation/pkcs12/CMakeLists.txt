# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do carregador de A1 (NL-2.2, ADR-S1). Um teste no CTest por slot (pkcs12.<nome>), sem vírgula no nome (L-45).
# Cada um depende da fixture de certificados (NL-2.1) e passa só com a linha "Totals: 3 passed, 0 failed" do QTest
# (initTestCase e cleanupTestCase contam como passes). Os dois slots de provedor ausente rodam em processo próprio.
find_package(OpenSSL 3.0 REQUIRED)

add_executable(tst_carregador_pkcs12 tst_carregador_pkcs12.cpp)
target_link_libraries(tst_carregador_pkcs12 PRIVATE Qt6::Core Qt6::Test nl_foundation_pkcs12
                      OpenSSL::Crypto nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
# Pasta vazia de módulos do OpenSSL: com OPENSSL_MODULES apontando para ela, o provider legacy não carrega.
set(NL_PROVEDORES_VAZIOS "${CMAKE_BINARY_DIR}/provedores_vazios")
file(MAKE_DIRECTORY "${NL_PROVEDORES_VAZIOS}")

foreach(nome IN ITEMS
    senhaCorretaCarrega
    senhaErradaDaErroProprio
    senhaErradaNoLegadoDaErroProprio
    cifraLegadaCarregaComProviderLegado
    legadoNaoDesligaODefault
    vencidoSinalizado
    folhaComValidadeFuturaReprova
    validadeIntermediariaVencidaReprova
    semEkuClientAuthSinalizado
    ekuMalformadaReprova
    credencialSemEkuMarcadaNoIndicador
    cnpjExtraidoDoCertificado
    cnpjAlfanumericoExtraidoComoTexto
    cpfExtraidoDoCertificado
    raizComparadaComoTexto
    inscricaoFormatoInvalidoRecusado
    semInscricaoSinalizada
    cnSemOidNaoViraInscricao
    otherNameDeOutroOidNaoViraInscricao
    cadeiaDeOutraRaizReprova
    cadeiaDeOutraSubordinadaReprova
    cadeiaIncompletaReprova
    cadeiaComSubordinadaCarrega
    pfxMalformadoReprova
    carregarZeraSenha
    ancorasDePemLemUmCertificado
    ancorasSemCertificadoReprovam
    senhaSeguraZeraConteudo
    descartarLiberaChave
    classificaMacComoSenhaErrada
    classificaUnsupportedDaBibliotecaEvp
    classificaMotivoDeOutraBibliotecaComoMalformado
    senhaComNulEhRecusada
    liberaZeraAntesDeLiberar
    ganchoSobreviveAoMover
    pfxSemMacComSenhaErradaFalhaFechado
    pfxSemMacComSenhaCertaCarrega
    pfxComChaveSemCifraRecusado
    chaveNaoConfereRecusada
    conferenciaDeChaveDistingueParesDeFixtures)
  add_test(NAME pkcs12.${nome} COMMAND tst_carregador_pkcs12 ${nome})
  set_tests_properties(pkcs12.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

foreach(nome IN ITEMS cifraLegadaIndisponivelSemProvider modernoNaoPrecisaDeLegadoSemProvider senhaErradaSemProvedorNaoTentaLegado)
  add_test(NAME pkcs12.${nome} COMMAND tst_carregador_pkcs12 ${nome})
  set_tests_properties(pkcs12.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};OPENSSL_MODULES=${NL_PROVEDORES_VAZIOS}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Carga das raízes da ICP-Brasil (NL-2.13, DA-2.47). Um teste no CTest por slot (pkcs12.raizes.<nome>), sem vírgula (L-45).
# Usa o repositório conferido em third_party (NL_ICP_RAIZES_DIR) e a AC de teste da fixture (certificados_teste).
add_executable(tst_carga_raizes_icp tst_carga_raizes_icp.cpp)
target_link_libraries(tst_carga_raizes_icp PRIVATE Qt6::Core Qt6::Test nl_foundation_pkcs12
                      OpenSSL::Crypto nl_warnings)

foreach(nome IN ITEMS
    raizesIcpCarregadasDoArquivoConferido
    hashDivergenteRecusaACarga
    cadeiaDeTesteNaoValidaContraAsRaizesIcp
    raizVencidaNaoEntraComoAncora
    raizForaDoPropositoNaoEntraComoAncora
    arquivoForaDoManifestoNaoEntra
    manifestoVazioRecusado
    metaComChaveRepetidaRecusada)
  add_test(NAME pkcs12.raizes.${nome} COMMAND tst_carga_raizes_icp ${nome})
  set_tests_properties(pkcs12.raizes.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_ICP_RAIZES_DIR=${NL_ICP_RAIZES_DIR};NL_ICP_RAIZES_MANIFESTO=${NL_ICP_RAIZES_MANIFESTO}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
