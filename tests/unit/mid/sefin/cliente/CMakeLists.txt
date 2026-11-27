# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do ClienteSefin (NL-2.8): política de retry, catálogo de erros, leitor de erro e cliente contra o servidor
# falso (tools/servidor_falso, pelo lançador com oráculo). Slots mid.sefin.cliente.<nome>, sem vírgula no nome (L-45).
find_package(OpenSSL 3.0 REQUIRED)
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

add_executable(tst_registro_transporte tst_registro_transporte.cpp)
target_link_libraries(tst_registro_transporte PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_foundation_transporte nl_warnings)
foreach(nome IN ITEMS eventoTemSoCamposFechados valorForaDoConjuntoVira_outro camposNovosSaoFechados versaoForaDoConjuntoVira_outro codigosSslLimitadosEm16)
  nl_registra_teste_qt(NAME mid.sefin.registroTransporte.${nome} COMMAND tst_registro_transporte ${nome})
  set_tests_properties(mid.sefin.registroTransporte.${nome} PROPERTIES
                       FAIL_REGULAR_EXPRESSION "FAIL!"
                       ENVIRONMENT "NL_LOGS_PII_CLI_DIR=${CMAKE_BINARY_DIR}/logs_pii_cli"
                       DEPENDS cli.limpaLogsPii)
endforeach()

# Evento sefin.resposta_sem_codigo (NL-3.L, passo 15): átomo puro, sem rede; roda também no build Flatpak.
add_executable(tst_registro_resposta_sem_codigo tst_registro_resposta_sem_codigo.cpp)
target_link_libraries(tst_registro_resposta_sem_codigo PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_log nl_warnings)
foreach(nome IN ITEMS eventoTemStatusTipoETamanho tipoConteudoSaiDoConjuntoFechado tipoConteudoForaDoConjuntoViraOutro semTipoConteudoViraAusente camposDoEventoNaoSaoProibidos)
  nl_registra_teste_qt(NAME mid.sefin.respostaSemCodigo.${nome} COMMAND tst_registro_resposta_sem_codigo ${nome})
  set_tests_properties(mid.sefin.respostaSemCodigo.${nome} PROPERTIES
                       FAIL_REGULAR_EXPRESSION "FAIL!"
                       ENVIRONMENT "NL_LOGS_PII_CLI_DIR=${CMAKE_BINARY_DIR}/logs_pii_cli"
                       DEPENDS cli.limpaLogsPii)
endforeach()

# Catálogo de erros da Sefin (NL-3.L): átomo puro, sem rede; roda também no build Flatpak.
add_executable(tst_catalogo_erros_sefin tst_catalogo_erros_sefin.cpp)
target_link_libraries(tst_catalogo_erros_sefin PRIVATE Qt6::Core Qt6::Test nl_mid nl_warnings)
foreach(nome IN ITEMS codigosDoEnderecoTemMensagemPropria e1235DizQueOLeiauteMudou codigoForaDoCatalogoFicaComAGenerica)
  nl_registra_teste_qt(NAME mid.sefin.catalogoErros.${nome} COMMAND tst_catalogo_erros_sefin ${nome})
  set_tests_properties(mid.sefin.catalogoErros.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()

add_executable(tst_cliente_sefin tst_cliente_sefin.cpp)
target_link_libraries(tst_cliente_sefin PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_mid nl_back nl_foundation_pkcs12
                      nl_foundation_transporte nl_foundation_log OpenSSL::Crypto nl_warnings)

set(NL_CERT_DIR "${CMAKE_BINARY_DIR}/certificados_teste")
if(NL_FLATPAK_BUILD)
    # DA-2.28: o SDK KDE 6.11 não traz xmlsec1. Os slots que assinam a DPS de referência pelo xmlsec1 (dpsAssinada)
    # ou sobem o servidor com oráculo ficam desligados aqui. A troca pelo AssinadorXml::assinarEnveloped fica na INBOX.
    message(STATUS "cliente Sefin: slots com xmlsec1 ou com o servidor de oráculo desligados no build Flatpak (NL_FLATPAK_BUILD=ON, DA-2.28)")
    set(NL_SLOTS_CLIENTE
        leitorAceitaErroEErrosECodigoECodigo
        catalogoDaMensagemDoCodigoE
        backoffDobraAteAPrimeiraRepeticao
        backoffNaoEstouraComExpoenteAlto)
else()
    find_program(NL_XMLLINT_CLIENTE xmllint REQUIRED)
    find_program(NL_XMLSEC1_CLIENTE xmlsec1 REQUIRED)
    set(NL_SLOTS_CLIENTE
        sucessoExtraiChaveDe50Posicoes
        falhaDePostVaiAoLogComEtapaPost
        falhaDeGetVaiAoLogComEtapaGet
        ancorasTlsVaziasSaiComCausaDeConfiguracao
        postTransitorioConsultaDpsAntesDeReenviar
        dps404E2404LiberaReenvio
        conflito409ConsultaENaoReenvia
        rejeicaoE0014NaoRepete
        rejeicaoE0244NaoRepeteNemConsulta
        certificadoRevogadoMapeadoPelaRejeicaoDaSefin
        textoLivreDaApiComCpfSoLogaCodigoE
        getTransitorioRepeteComBackoff
        leitorAceitaErroEErrosECodigoECodigo
        catalogoDaMensagemDoCodigoE
        backoffDobraAteAPrimeiraRepeticao
        xmlNfseGuardaXmlDescomprimido
        base64InvalidoRecusado
        xmlIlegivelNumSucessoNaoLiberaReenvio
        backoffNaoEstouraComExpoenteAlto
        posPost2xxSemChaveConsultaDps
        posPost2xxForaDe200e201ConsultaDps
        post3xxConsultaDpsAntesDeReenviar
        erroDefinitivoDepoisDoEnvioViraIndeterminado
        consultaComCodigoQueNaoE2404ViraIndeterminado
        naoGeradaReenviaMesmoIdNuncaNumeroNovo
        e0014NoReenvioConsultaEDevolveAChave
        conexaoFechadaAntesDoTlsSemBytesEhTransitoria
        recusaNoHandshakeTls12NaoEscreveCorpoESemConsulta
        corpoEscritoEConexaoFechadaConsultaAntesDeResponder
        corpoEscritoComRstConsultaAntesDeResponder
        recusaDoCertificadoNoTls13NuncaReenviaAsCegas
        conflito409ComConsultaSemResolverViraIndeterminado
        base64ComIgualNoMeioRecusado
        conflito409ComMesmoInfDpsRecuperaANota
        conflito409ComInfDpsDiferenteViraDpsDuplicada
        codigoEComCpfNaoChegaAoLog
        codigoEForaDoFormatoFicaVazio
        base64ComCaractereInvalidoETamanhoValidoRecusado
        membroExtraDoGzipViraXmlPendente
        comparadorRealIgualParaMesmaDps
        comparadorRealDiferenteParaDpsAlterada
        e0014ComComparadorRealRecuperaANota
        observadorRecebeCadaRespostaDaEmissao
        e0014NaPrimeiraTentativaReconcilia
        e0014ComMesmoInfDpsRecuperaANota
        e0014ComInfDpsDiferenteViraDpsDuplicada
        e0014SemConseguirCompararViraComparacaoIndisponivel)
endif()
set(NL_AMBIENTE_CLIENTE
    "NL_CERTIFICADOS_DIR=${NL_CERT_DIR};NL_FIXTURES_DPS=${CMAKE_SOURCE_DIR}/tests/fixtures/dps;NL_LANCADOR=${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py;NL_PYTHON3=${Python3_EXECUTABLE};NL_XMLLINT=${NL_XMLLINT_CLIENTE};NL_XMLSEC1=${NL_XMLSEC1_CLIENTE};NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR}")

foreach(nome IN LISTS NL_SLOTS_CLIENTE)
  nl_registra_teste_qt(NAME mid.sefin.cliente.${nome} COMMAND tst_cliente_sefin ${nome})
  set_tests_properties(mid.sefin.cliente.${nome} PROPERTIES
                       FIXTURES_REQUIRED certificados_teste
                       ENVIRONMENT "${NL_AMBIENTE_CLIENTE}"
                       FAIL_REGULAR_EXPRESSION "FAIL!"
                       TIMEOUT 120)  # teto explícito por slot: sem ele o CTest espera 1500 s por teste travado
endforeach()

# Falhas de envio do cliente Sefin (NL-3.C; S5b, DA-3C.22): os 16 slots da causa tipada, em executável próprio, e o 403 sem
# código da NL-3.L (passo 15). Mesmo nome de
# CTest (mid.sefin.cliente.<nome>) e mesmas propriedades do laço de cima. Fora do build Flatpak, como eles (DA-2.28).
if(NOT NL_FLATPAK_BUILD)
    add_executable(tst_cliente_sefin_falhas_envio tst_cliente_sefin_falhas_envio.cpp)
    target_link_libraries(tst_cliente_sefin_falhas_envio PRIVATE Qt6::Core Qt6::Network Qt6::Test nl_mid nl_back
                          nl_foundation_pkcs12 nl_foundation_transporte nl_foundation_log OpenSSL::Crypto nl_warnings)
    foreach(nome IN ITEMS
        consultaDps200SemChaveDaRespostaSemCodigo
        consultaDps200NaoObjetoDaRespostaSemCodigo
        consultaNota200NaoObjetoDaRespostaSemCodigo
        consultaDps404SemCodigoValidoDaRespostaSemCodigo
        consultaNotaInexistenteDaRespostaSemCodigo
        consultaDpsSemConexaoDaRede
        consultaDpsRecusadaNoTls12DaCertificadoProvavel
        consultaDps5xxPersistenteDaSefinIndisponivel
        consultaSemTentativaNaoEnviaNada
        naoGeradaSemTentativaDaNaoConcluido
        consulta5xxDepoisDoEnvioPropagaACausa
        conflito409ComConsulta5xxPropagaACausa
        emitirSemTentativaNaoEnviaNada
        servidorDeOutraRaizSemBytesDaServidorNaoConfirmado
        zeroBytesNoReenvioDepoisDeE2404NaoDizNadaFoiEnviado
        conflito409ComNotaIndisponivelPropagaACausa
        status403HtmlSemCodigoDefinitivoSemNovaTentativa)
      nl_registra_teste_qt(NAME mid.sefin.cliente.${nome} COMMAND tst_cliente_sefin_falhas_envio ${nome})
      set_tests_properties(mid.sefin.cliente.${nome} PROPERTIES
                           FIXTURES_REQUIRED certificados_teste
                           ENVIRONMENT "${NL_AMBIENTE_CLIENTE}"
                           FAIL_REGULAR_EXPRESSION "FAIL!"
                           TIMEOUT 120)
    endforeach()
endif()

# Falhas do envio (NL-3.C, plano 3.3 e 4.2): átomo puro, causa tipada, sem rede. Slots mid.sefin.falhasDoEnvio.<nome>.
add_executable(tst_falhas_do_envio tst_falhas_do_envio.cpp)
target_link_libraries(tst_falhas_do_envio PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_transporte nl_warnings)
foreach(nome IN ITEMS
    causaDoTransporteCobreTodasAsCausas
    semBytesTlsDeixaDeSerTransitoria
    semBytesRedeContinuaTransitoria
    semBytesConfiguracaoViraConfiguracaoDaConexao
    falhaDeRedeNaConsultaLevaACausa
    semBytesNoReenvioViraNaoConcluido
    indeterminadoDepoisDaConsultaLevaACausa)
  nl_registra_teste_qt(NAME mid.sefin.falhasDoEnvio.${nome} COMMAND tst_falhas_do_envio ${nome})
  set_tests_properties(mid.sefin.falhasDoEnvio.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
