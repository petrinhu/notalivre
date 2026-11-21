# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mid/persistencia (NL-3.1a, bloco M-R): política de conexão por família de servidor. Alvo próprio, um átomo, uma razão.
# Depende da Foundation sql (DadosConexao) e do SenhaSegura da pkcs12, para baixo. O cabeçalho público fica em src/mid/include.
add_library(nl_mid_persistencia STATIC politica_conexao.cpp)
target_include_directories(nl_mid_persistencia PUBLIC ${CMAKE_SOURCE_DIR}/src/mid/include)
target_link_libraries(nl_mid_persistencia PUBLIC nl_foundation_base nl_foundation_sql nl_foundation_pkcs12)
target_link_libraries(nl_mid_persistencia PRIVATE nl_warnings)
