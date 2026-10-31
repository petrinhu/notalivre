# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da NL-2.1: fixtures de certificados de teste. O gerador roda como fixture do CTest (FIXTURES_SETUP),
# em ${CMAKE_BINARY_DIR}/certificados_teste, dentro do contêiner. Nada é versionado.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
set(NL_CERT_GERADOR "${CMAKE_CURRENT_SOURCE_DIR}/gerar_certificados.py")
set(NL_CERT_TESTES "${CMAKE_CURRENT_SOURCE_DIR}/test_certificados.py")

add_test(NAME certificados.gerar
         COMMAND Python3::Interpreter -I "${NL_CERT_GERADOR}")
set_tests_properties(certificados.gerar PROPERTIES FIXTURES_SETUP certificados_teste
                     ENVIRONMENT "NL_CERT_DIR=${NL_CERT_DIR}")

foreach(nome IN ITEMS
    fixturesGeradasSaoLidasPeloOpenssl
    pfxLegadoFalhaSemLegacyComUnsupported
    pfxModernoUsaPbes2Aes256
    pfxLegadoUsaRc2EDes
    cnpjNoOtherNameConfereComOid
    cpfNoOtherNameNasPosicoes9a19
    ekuClientAuthSoNoCertificadoDeCliente
    vencidoRejeitadoPeloVerify
    revogadoRejeitadoPorLcr
    subAcEhCaPathlenZeroAssinadaPelaAc
    cnpjSubVemDaSubAc
    cnpjSubSemIntermediariaSemCa
    cadeiaForaDaAcEhAutoassinadaSemCa
    semIdentificacaoSemOids
    cnpjsDeTesteTemDvValido
    servidorFalsoTemSanEEkuServerAuth
    pfxTemCadeiaDaAc)
  add_test(NAME certificados.${nome}
           COMMAND Python3::Interpreter -I "${NL_CERT_TESTES}" ${nome})
  set_tests_properties(certificados.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "NL_CERTIFICADOS_DIR=${NL_CERT_DIR}"
                       PASS_REGULAR_EXPRESSION "Ran 1 test in"
                       FAIL_REGULAR_EXPRESSION "FAILED")
endforeach()

# Não usam a fixture: o gerador roda dentro do próprio teste (duas gerações), ou o teste só lê vetores.
foreach(nome IN ITEMS senhaAleatoriaPorExecucao senhaNasceCom0600 dvAlfanumericoVetoresOficiais)
  add_test(NAME certificados.${nome}
           COMMAND Python3::Interpreter -I "${NL_CERT_TESTES}" ${nome})
  # Passa só se o unittest rodou exatamente um caso (linha "Ran 1 test in") e não houve FAILED.
  # Sem isso, uma suíte vazia ("Ran 0 tests", OK) dava verde com RC 0.
  set_tests_properties(certificados.${nome} PROPERTIES
                       PASS_REGULAR_EXPRESSION "Ran 1 test in"
                       FAIL_REGULAR_EXPRESSION "FAILED")
endforeach()
