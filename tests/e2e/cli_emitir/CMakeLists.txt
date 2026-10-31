# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# E2E de `notalivre emitir` contra o servidor falso (NL-2.9, DA-2.44, DA-2.45) e os casos de recusa do plano. Só fixtures
# sintéticas. Só em contêiner. A saída de cada caso vai para a pasta de logs da CLI, que o portão de PII varre (ADR-0005 e).
# Build Flatpak (NL_FLATPAK_BUILD=ON, DA-2.28): o SDK KDE 6.11 não traz xmlsec1, e este E2E depende do xmlsec1 e do servidor
# falso com oráculo. Fica desligado no Flatpak, como o servidor falso e o cliente Sefin; no build nativo nada muda.
if(NL_FLATPAK_BUILD)
    message(STATUS "e2e cli_emitir: desligado no build Flatpak (NL_FLATPAK_BUILD=ON, DA-2.28): depende de xmlsec1 e do servidor falso")
    return()
endif()

find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)
find_program(NL_XMLLINT_EMITIR xmllint REQUIRED)
find_program(NL_XMLSEC1_EMITIR xmlsec1 REQUIRED)

add_executable(tst_cli_emitir tst_cli_emitir.cpp)
add_dependencies(tst_cli_emitir notalivre-cli)
target_link_libraries(tst_cli_emitir PRIVATE Qt6::Core Qt6::Test util nl_warnings nl_teste_apoio)
target_compile_definitions(tst_cli_emitir PRIVATE
    "NL_CLI_EMITIR=\"$<TARGET_FILE:notalivre-cli>\""
    "NL_LANCADOR_EMITIR=\"${CMAKE_SOURCE_DIR}/tools/servidor_falso/lancador.py\""
    "NL_PYTHON3_EMITIR=\"${Python3_EXECUTABLE}\""
    "NL_CERTIFICADOS_EMITIR=\"${CMAKE_BINARY_DIR}/certificados_teste\""
    "NL_ENTRADAS_EMITIR=\"${CMAKE_SOURCE_DIR}/tests/e2e/dps_validar/entradas\""
    "NL_XSD_EMITIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_MANIFESTO_EMITIR=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_TABELA_SHA1_EMITIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures/xmldsig/tabela_sha1_por_distro.txt\"")

set(NL_SEGREDOS_PII_REF "${CMAKE_BINARY_DIR}/segredos_pii_execucao.txt")
set(NL_AMBIENTE_E2E_EMITIR
    "NL_XMLLINT=${NL_XMLLINT_EMITIR};NL_XMLSEC1=${NL_XMLSEC1_EMITIR};NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR};NL_LOGS_PII_CLI_DIR=${CMAKE_BINARY_DIR}/logs_pii_cli;NL_SEGREDOS_PII_REF=${NL_SEGREDOS_PII_REF}")

foreach(nome IN ITEMS
    cliEmiteDpsAceitaPeloServidorFalso
    cliRecusaProducaoBloqueada
    cliRecusaProducaoSemCnpjDigitado
    senhaNaoApareceEmArgvNemLog
    cliRecusaDpsInvalidaSemPost
    e0014DaMesmaDpsRecuperaAChave
    e0014ComInfDpsDiferenteSaiComCinco
    e0014SemComparacaoSaiComSete
    respostaCruaGravadaParaAnalise)
    nl_registra_teste_qt(NAME e2e.cliEmitir.${nome} COMMAND tst_cli_emitir ${nome})
    set_tests_properties(e2e.cliEmitir.${nome} PROPERTIES
                         FIXTURES_REQUIRED certificados_teste
                         DEPENDS cli.limpaLogsPii
                         ENVIRONMENT "${NL_AMBIENTE_E2E_EMITIR}"
                         PASS_REGULAR_EXPRESSION "Totals: [0-9]+ passed, 0 failed, 0 skipped"
                         FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
