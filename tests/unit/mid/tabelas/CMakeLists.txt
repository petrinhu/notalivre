# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Adaptador do catálogo de localidades (NL-3.M, DA-3M.4). Um teste no CTest por slot (mid.tabelas.<nome>), sem vírgula no nome (L-45).
# As tabelas de exemplo são gravadas pelo próprio teste, numa pasta temporária; nada versionado em tests/fixtures.
# O slot tabelasDoBuildCarregam lê as tabelas reais do build (NL_TABELAS_DIR) com o pino do configure (NL_TABELAS_MANIFESTO_SHA256).
add_executable(tst_catalogo_localidades tst_catalogo_localidades.cpp)
target_link_libraries(tst_catalogo_localidades PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_base nl_warnings)
target_compile_definitions(tst_catalogo_localidades PRIVATE
    "NL_TABELAS_DIR=\"${NL_TABELAS_DIR}\""
    "NL_TABELAS_MANIFESTO_SHA256=\"${NL_TABELAS_MANIFESTO_SHA256}\"")

foreach(nome IN ITEMS
    fixtureCarregaERespondeExistencia
    manifestoForaDoPinRecusado
    tabelaDivergenteDoManifestoRecusada
    tabelaAdulteradaEMalformadaRecusadaPeloHash
    tabelaAusenteRecusada
    jsonMalformadoRecusado
    manifestoMalformadoRecusado
    manifestoComComentarioCarrega
    tabelasDoBuildCarregam)
    nl_registra_teste_qt(NAME mid.tabelas.${nome} COMMAND tst_catalogo_localidades ${nome})
    set_tests_properties(mid.tabelas.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
