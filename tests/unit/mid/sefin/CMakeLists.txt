# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do codec XML da DPS (NL-1.6b): QtTest sobre os tipos do Back, validação pelo ValidadorXsd e oráculo xmllint.
# Fixtures em tests/fixtures/dps: saída do próprio codec, byte a byte (o QtTest fixturesDpsIguaisAoCodec confere).
# Regeração (no contêiner, como a suíte): tools/conteiner/build_local.sh fedora44 debug roda o CTest gera_fixtures_dps,
# que grava em <build>/fixtures_dps. Depois, revisar o diff e copiar: cp <build>/fixtures_dps/*.xml tests/fixtures/dps/
# (o build do contêiner fica em /var/tmp/notalivre-build-fedora44/debug no host).
add_executable(tst_codec_dps tst_codec_dps.cpp)
# Os limites da pasta do leiaute são lidos direto pelo teste (kLimiteXDescServ, kVersaoLeiaute), sem constante copiada.
target_include_directories(tst_codec_dps PRIVATE ${CMAKE_SOURCE_DIR}/src/mid/sefin/leiaute/prodrest_20261020)
target_link_libraries(tst_codec_dps PRIVATE Qt6::Test nl_mid nl_back nl_foundation nl_warnings)
target_compile_definitions(tst_codec_dps PRIVATE
    "NL_XSD_PRODREST_DIR=\"${NL_XSD_PRODREST_DIR}\""
    "NL_XSD_PRODREST_MANIFESTO=\"${NL_XSD_PRODREST_MANIFESTO}\""
    "NL_XSD_PRODREST_RAIZ=\"${NL_XSD_PRODREST_RAIZ}\""
    "NL_FIXTURES_DPS=\"${CMAKE_SOURCE_DIR}/tests/fixtures/dps\"")
add_test(NAME tst_codec_dps COMMAND tst_codec_dps)

# Geração das fixtures (fora da suíte normal): grava os XML do codec no diretório de build, de onde são copiados para a árvore.
add_test(NAME gera_fixtures_dps COMMAND tst_codec_dps geraFixturesDps)
set_tests_properties(gera_fixtures_dps PROPERTIES ENVIRONMENT "NL_FIXTURES_DPS_SAIDA=${CMAKE_BINARY_DIR}/fixtures_dps")

# Oráculo independente: xmllint --schema sobre as mesmas fixtures; o mesmo script do validador da NL-1.6a (régua por nome).
find_program(NL_XMLLINT_DPS xmllint REQUIRED)
add_test(NAME oraculo_xmllint_dps
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/unit/foundation/xml/oraculo_xmllint.sh"
        "${NL_XMLLINT_DPS}" "${NL_XSD_PRODREST_DIR}" "${CMAKE_SOURCE_DIR}/tests/fixtures/dps")
