# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do codec XML da DPS (NL-1.6b): QtTest sobre os tipos do Back, validação pelo ValidadorXsd e oráculo xmllint.
# Fixtures em tests/fixtures/dps: saída do próprio codec, byte a byte (o QtTest fixturesDpsIguaisAoCodec confere).
# Regeração (fora do CTest, DA-2.61): cmake --build <build> --target gera_fixtures_dps, depois
# <build>/tests/tools/gera_fixtures_dps/gera_fixtures_dps <pasta de saída>. Revisar o diff e copiar para tests/fixtures/dps/.
add_executable(tst_codec_dps tst_codec_dps.cpp)
# Os limites da pasta do leiaute são lidos direto pelo teste (kLimiteXDescServ, kVersaoLeiaute), sem constante copiada.
target_include_directories(tst_codec_dps PRIVATE ${CMAKE_SOURCE_DIR}/src/mid/sefin/leiaute/prodrest_20261020)
target_link_libraries(tst_codec_dps PRIVATE Qt6::Test nl_mid nl_back nl_foundation nl_warnings)
target_compile_definitions(tst_codec_dps PRIVATE
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_XSD_PRODREST_RAIZ=\"${NL_XSD_PRODREST_RAIZ}\""
    "NL_FIXTURES_DPS=\"${CMAKE_SOURCE_DIR}/tests/fixtures/dps\"")
nl_registra_teste_qt(NAME tst_codec_dps COMMAND tst_codec_dps)

# Envelope da DPS (NL-2.4, DA-2.4): INFERÊNCIA até a NL-2.10. Só codifica o GZip que o chamador entrega; o GZip fica
# na fatia própria dele. O teste lê a chave e o Content-Type da pasta do leiaute, pelo include público de nl_mid.
add_executable(tst_envelope_dps tst_envelope_dps.cpp)
target_link_libraries(tst_envelope_dps PRIVATE Qt6::Core Qt6::Test nl_mid nl_foundation_base nl_warnings)
nl_registra_teste_qt(NAME tst_envelope_dps COMMAND tst_envelope_dps)

# Oráculo independente: xmllint --schema sobre as mesmas fixtures; o mesmo script do validador da NL-1.6a (régua por nome).
find_program(NL_XMLLINT_DPS xmllint REQUIRED)
find_program(NL_BASH_SEFIN bash REQUIRED)
add_test(NAME oraculo_xmllint_dps
    COMMAND "${NL_BASH_SEFIN}" "${CMAKE_SOURCE_DIR}/tests/unit/foundation/xml/oraculo_xmllint.sh" "${NL_XMLLINT_DPS}")
set_tests_properties(oraculo_xmllint_dps PROPERTIES
    ENVIRONMENT "NL_XSD_DIR=${NL_XSD_PRODREST_DIR};NL_FIXTURES_ORACULO=${CMAKE_SOURCE_DIR}/tests/fixtures/dps")

# Cliente Sefin e política de retry (NL-2.8): subpasta própria.
add_subdirectory(cliente)

# Adaptador de assinatura da DPS (NL-2.9, DA-2.42): subpasta própria.
add_subdirectory(assinatura)
