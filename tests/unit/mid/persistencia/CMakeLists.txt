# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de unidade da política de conexão do Mid (NL-3.1a, bloco M-R): sem servidor.
add_executable(tst_politica_conexao tst_politica_conexao.cpp)
target_link_libraries(tst_politica_conexao PRIVATE Qt6::Test nl_mid_persistencia nl_foundation_pkcs12 nl_warnings)
nl_registra_teste_qt(NAME tst_politica_conexao COMMAND tst_politica_conexao)
