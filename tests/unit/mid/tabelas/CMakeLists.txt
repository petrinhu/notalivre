# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Adaptador do catálogo de localidades (NL-3.M, DA-3M.4). Um teste no CTest por slot (mid.tabelas.<nome>), sem vírgula no nome (L-45).
# As tabelas de exemplo são gravadas pelo próprio teste, numa pasta temporária; nada versionado em tests/fixtures.
# O slot tabelasDoBuildCarregam (tabelas reais do build) fica para o passo 1 de dados, quando elas existirem no build.
add_executable(tst_catalogo_localidades tst_catalogo_localidades.cpp)
target_link_libraries(tst_catalogo_localidades PRIVATE Qt6::Core Qt6::Test nl_mid nl_back nl_foundation_base nl_warnings)

foreach(nome IN ITEMS
    fixtureCarregaERespondeExistencia
    manifestoForaDoPinRecusado
    tabelaDivergenteDoManifestoRecusada
    tabelaAusenteRecusada
    jsonMalformadoRecusado
    manifestoMalformadoRecusado)
    nl_registra_teste_qt(NAME mid.tabelas.${nome} COMMAND tst_catalogo_localidades ${nome})
    set_tests_properties(mid.tabelas.${nome} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")
endforeach()
