# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/sql (NL-3.1a, bloco U-R): executor SQL só com parâmetros vinculados (CONTRACT 12.1). Motor agnóstico:
# não conhece família de servidor, emissor nem nota. O Connector/C é PRIVATE: nenhum tipo dele sai dos cabeçalhos públicos.
# Esta fase traz os cabeçalhos públicos e stubs que devolvem sql.nao_implementado; o corpo entra no U-G e no I-G.
include("${CMAKE_SOURCE_DIR}/src/foundation/sql/nl_conector_mariadb.cmake")
add_library(nl_foundation_sql STATIC
    parametro.cpp
    validacao.cpp
    opcoes_sessao.cpp
    modo_aberto.cpp
    modo_fixado.cpp
    conexao.cpp
    vinculos.cpp
    preparado.cpp
    leitura.cpp
    transacao.cpp
    erros.cpp
    executor.cpp)
target_include_directories(nl_foundation_sql PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(nl_foundation_sql PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/privado ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(nl_foundation_sql PUBLIC nl_foundation_base Qt6::Core)
target_link_libraries(nl_foundation_sql PRIVATE ${NL_MARIADB_LIB} nl_warnings)
