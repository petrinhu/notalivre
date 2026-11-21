# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/sql (NL-3.1a, bloco B): alvo vazio que prova o encanamento do Connector/C em todo preset e no Flatpak.
# O prefixo vem da guarda do Connector/C deste diretório (src/foundation/sql/nl_conector_mariadb.cmake), que os testes de banco
# também usam. O motor de verdade entra no bloco U.
include("${CMAKE_SOURCE_DIR}/src/foundation/sql/nl_conector_mariadb.cmake")
add_library(nl_foundation_sql STATIC sql_vazio.cpp)
target_include_directories(nl_foundation_sql PRIVATE ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(nl_foundation_sql PRIVATE ${NL_MARIADB_LIB} nl_warnings)
