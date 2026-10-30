# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do servidor falso da API (NL-2.6). Cada caso vira um add_test próprio, rodado com python3 -I.
# O ambiente leva o xmllint, o xmlsec1 e o XSD de Produção Restrita, e a pasta de trabalho do build.
# Build Flatpak (NL_FLATPAK_BUILD=ON, fixado no manifesto): o SDK KDE 6.11 não traz xmlsec1 (CI 40804430962). A opção
# desliga estes testes, e o configure diz isso no log. Nunca se desliga por falta da ferramenta: no build nativo, o
# find_program REQUIRED abaixo continua reprovando o configure se o xmlsec1 faltar.
if(NL_FLATPAK_BUILD)
    message(STATUS "servidor falso: testes desligados neste build Flatpak (NL_FLATPAK_BUILD=ON, DA-2.28)")
    return()
endif()

find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)
find_program(NL_XMLLINT_SF xmllint REQUIRED)
find_program(NL_XMLSEC1_SF xmlsec1 REQUIRED)

set(NL_TESTE_SERVIDOR_FALSO "${CMAKE_CURRENT_SOURCE_DIR}/test_servidor_falso.py")
set(NL_AMBIENTE_SERVIDOR_FALSO
    "NL_XMLLINT=${NL_XMLLINT_SF};NL_XMLSEC1=${NL_XMLSEC1_SF};NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR};NL_SF_DIR=${CMAKE_BINARY_DIR}/servidor_falso")

# Arnês do fedora44 (DA-2.10, ADR 0007): só nessa distro a variável de SHA-1 entra no ambiente dos testes, e o
# configure escreve a linha no log do job. O ctest esconde a saída de testes que passam, por isso não é o teste que a imprime.
file(STRINGS "/etc/os-release" NL_OS_LINHAS)
foreach(linha IN LISTS NL_OS_LINHAS)
    if(linha MATCHES "^ID=(.*)$")
        set(NL_SO_ID "${CMAKE_MATCH_1}")
    endif()
    if(linha MATCHES "^VERSION_ID=(.*)$")
        set(NL_SO_VERSAO "${CMAKE_MATCH_1}")
    endif()
endforeach()
string(REPLACE "\"" "" NL_SO_VERSAO "${NL_SO_VERSAO}")
if(NL_SO_ID STREQUAL "fedora" AND NL_SO_VERSAO STREQUAL "44")
    message(STATUS "servidor falso: arnês fedora44 ativo, OPENSSL_ENABLE_SHA1_SIGNATURES=1 (DA-2.10, ADR 0007)")
    string(APPEND NL_AMBIENTE_SERVIDOR_FALSO ";OPENSSL_ENABLE_SHA1_SIGNATURES=1")
endif()

foreach(nome IN ITEMS
    dpsValidaRecebeChave50
    dpsAdulteradaRejeitada
    assinaturaSha1AceitaPeloOraculo
    semCertificadoRecusado
    falhaRoteirizadaAconteceUmaVez
    variavelSha1SoNoFedora44
    ambienteDoXmlsec1SoNoFedora44
    envelopeZlibCruRecusadoCom400
    tlsMinimoTls12RecusaTls11
    escutaSoEmLoopback
    verificacaoLaxObrigatoriaNa13
    oraculoSemAssinarSemChamador
    rotasDeEventoEDfeRespondem501
    verificacaoPadraoSemLax
    rejeicaoRoteirizadaDevolveCodigoE
    corpoTruncadoFechaNoMeio
    ecoAcceptEncodingDaFalha
    contentEncodingGzipDaFalha
    redirecionaRoteirizado
    corpoSemTamanhoFechaNoCorpo
    nfseDefeituosaNoSucessoDoPostChegaComChaveEFormaErrada
    nfseDefeituosaSoValeParaPostNfse)
  add_test(NAME servidor_falso.${nome}
           COMMAND Python3::Interpreter -I "${NL_TESTE_SERVIDOR_FALSO}" "test_${nome}")
  set_tests_properties(servidor_falso.${nome} PROPERTIES ENVIRONMENT "${NL_AMBIENTE_SERVIDOR_FALSO}")
endforeach()
