# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do servidor falso da API (NL-2.6). Cada caso vira um add_test próprio, rodado com python3 -I.
# O ambiente leva o xmllint, o xmlsec1 e o XSD de Produção Restrita, e a pasta de trabalho do build.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)
find_program(NL_XMLLINT_SF xmllint REQUIRED)
find_program(NL_XMLSEC1_SF xmlsec1 REQUIRED)

set(NL_TESTE_SERVIDOR_FALSO "${CMAKE_CURRENT_SOURCE_DIR}/test_servidor_falso.py")
set(NL_AMBIENTE_SERVIDOR_FALSO
    "NL_XMLLINT=${NL_XMLLINT_SF};NL_XMLSEC1=${NL_XMLSEC1_SF};NL_XSD_PRODREST_DIR=${NL_XSD_PRODREST_DIR};NL_SF_DIR=${CMAKE_BINARY_DIR}/servidor_falso")

foreach(nome IN ITEMS
    dpsValidaRecebeChave50
    dpsAdulteradaRejeitada
    assinaturaSha1AceitaPeloOraculo
    semCertificadoRecusado
    falhaRoteirizadaAconteceUmaVez
    variavelSha1SoNoFedora44)
  add_test(NAME servidor_falso.${nome}
           COMMAND Python3::Interpreter -I "${NL_TESTE_SERVIDOR_FALSO}" "test_${nome}")
  set_tests_properties(servidor_falso.${nome} PROPERTIES ENVIRONMENT "${NL_AMBIENTE_SERVIDOR_FALSO}")
endforeach()
