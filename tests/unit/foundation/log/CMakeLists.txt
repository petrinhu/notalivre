# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do Log da Foundation (NL-2.7, ADR-S4). tst_log grava a saída em NL_LOGS_PII_DIR; o portão portao_pii_logs
# varre essa pasta com varredura_logs.py, numa só invocação, e exige zero ocorrências com piso não vazio (L-36).
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

set(NL_LOGS_PII_DIR "${CMAKE_BINARY_DIR}/logs_pii")
set(NL_VARREDURA_LOGS "${CMAKE_CURRENT_SOURCE_DIR}/varredura_logs.py")
set(NL_TERMOS_PII "${CMAKE_SOURCE_DIR}/tests/fixtures/pii/referencia.txt")

add_executable(tst_log tst_log.cpp)
target_link_libraries(tst_log PRIVATE Qt6::Core Qt6::Test nl_foundation_log nl_warnings)
target_compile_definitions(tst_log PRIVATE
    "NL_LOGS_PII_DIR=\"${NL_LOGS_PII_DIR}\""
    "NL_FIXTURES_PII=\"${CMAKE_SOURCE_DIR}/tests/fixtures/pii\""
    "NL_PYTHON3=\"${Python3_EXECUTABLE}\""
    "NL_VARREDURA_LOGS=\"${NL_VARREDURA_LOGS}\"")
nl_registra_teste_qt(NAME tst_log COMMAND tst_log)

add_test(NAME portao_pii_logs
    COMMAND ${Python3_EXECUTABLE} -I "${NL_VARREDURA_LOGS}" --dir "${NL_LOGS_PII_DIR}" --termos "${NL_TERMOS_PII}")
set_tests_properties(portao_pii_logs PROPERTIES DEPENDS tst_log)

# Máscara da chave e do Id da NFS-e (NL-2.8, DA-2.21): slots foundation.log.<nome>, sem vírgula no nome (L-45).
add_executable(tst_mascara_chave tst_mascara_chave.cpp)
target_link_libraries(tst_mascara_chave PRIVATE Qt6::Core Qt6::Test nl_foundation_log nl_warnings)
foreach(nome IN ITEMS
    chaveDeAcessoLogadaComInscricaoMascarada
    idNfseLogadoComInscricaoMascarada
    chaveForaDoLeiauteDescartada)
  nl_registra_teste_qt(NAME foundation.log.${nome} COMMAND tst_mascara_chave ${nome})
  set_tests_properties(foundation.log.${nome} PROPERTIES
                       PASS_REGULAR_EXPRESSION "Totals: 3 passed, 0 failed"
                       FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
