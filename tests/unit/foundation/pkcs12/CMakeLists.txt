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
    folhaComDataIlegivelReprova
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
  nl_registra_teste_qt(NAME pkcs12.${nome} COMMAND tst_carregador_pkcs12 ${nome})
  set_tests_properties(pkcs12.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

foreach(nome IN ITEMS cifraLegadaIndisponivelSemProvider modernoNaoPrecisaDeLegadoSemProvider senhaErradaSemProvedorNaoTentaLegado)
  nl_registra_teste_qt(NAME pkcs12.${nome} COMMAND tst_carregador_pkcs12 ${nome})
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
    arquivoNaoDeclaradoNoDiretorioRecusado
    manifestoVazioRecusado
    metaComChaveRepetidaRecusada
    arquivoAusenteRecusa
    certificadoIlegivelRecusa
    manifestoAusenteRecusa
    manifestoMalformadoRecusa
    nenhumaRaizAceitaRecusa
    raizAindaNaoValidaNaoEntraComoAncora
    derComCaixaAltaNaoDeclaradoRecusado
    raizNoInstanteExatoDoFimFicaVencida
    raizNoInstanteExatoDoInicioFicaVigente
    raizComDataInvalidaEhIlegivel)
  nl_registra_teste_qt(NAME pkcs12.raizes.${nome} COMMAND tst_carga_raizes_icp ${nome})
  set_tests_properties(pkcs12.raizes.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_ICP_RAIZES_DIR=${NL_ICP_RAIZES_DIR};NL_ICP_RAIZES_MANIFESTO=${NL_ICP_RAIZES_MANIFESTO}"
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

# Vazamento do contexto OpenSSL no carregamento (NL-3.0h, causa B). A checagem é in-process (LeakSanitizer), e o
# resultado vale só sob asan-ubsan: fora dele o teste carrega os dois .pfx e passa sem medir vazamento.
add_executable(tst_vazamento_ossl tst_vazamento_ossl.cpp)
target_link_libraries(tst_vazamento_ossl PRIVATE Qt6::Core Qt6::Test nl_foundation_pkcs12 OpenSSL::Crypto nl_warnings)
nl_registra_teste_qt(NAME pkcs12.decifrarNaoDeixaProvedorVivo COMMAND tst_vazamento_ossl)
set_tests_properties(pkcs12.decifrarNaoDeixaProvedorVivo PROPERTIES
                     FIXTURES_REQUIRED certificados_teste
                     ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                     PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                     FAIL_REGULAR_EXPRESSION "FAIL!")

# Interior da SenhaSegura com libsodium (NL-3.2c, ADR-0002). Um teste no CTest por slot (pkcs12.senha.<nome>), sem vírgula
# (L-45). Sem fixture de certificado: a senha é sintética. Os slots de falha entram pelo AcessoDeTesteSenhaSegura.
find_package(PkgConfig REQUIRED)
pkg_check_modules(NL_LIBSODIUM_TESTE REQUIRED IMPORTED_TARGET libsodium)
add_executable(tst_senha_segura tst_senha_segura.cpp)
target_link_libraries(tst_senha_segura PRIVATE Qt6::Core Qt6::Test nl_foundation_pkcs12 PkgConfig::NL_LIBSODIUM_TESTE nl_warnings)

foreach(nome IN ITEMS
    tentarDevolveErroSemMemoriaComum
    deLancaEmFalhaDeAlocacao
    senhaVaziaFunciona
    comoCTerminaEmNul
    sodiumInitFalhoNaoAlocaEmMemoriaComum
    bufferTemPaginaDeGuardaDepois
    tentarInicializaOSodium)
  nl_registra_teste_qt(NAME pkcs12.senha.${nome} COMMAND tst_senha_segura ${nome})
  # QTest soma initTestCase e cleanupTestCase implícitos: um slot sem falha sai com "Totals: 3 passed, 0 failed", como os vizinhos.
  set_tests_properties(pkcs12.senha.${nome} PROPERTIES
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
