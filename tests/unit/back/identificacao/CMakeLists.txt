# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários da identificação fiscal (NL-1.1): CNPJ, CPF, inscrição, código de município e CEP. Regra pura, sem I/O.
# Um executável por tipo, cada um no CTest pelo próprio nome.
add_executable(tst_cnpj tst_cnpj.cpp)
target_link_libraries(tst_cnpj PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_cnpj COMMAND tst_cnpj)

add_executable(tst_cpf tst_cpf.cpp)
target_link_libraries(tst_cpf PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_cpf COMMAND tst_cpf)

add_executable(tst_inscricao tst_inscricao.cpp)
target_link_libraries(tst_inscricao PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_inscricao COMMAND tst_inscricao)

add_executable(tst_codigo_municipio tst_codigo_municipio.cpp)
target_link_libraries(tst_codigo_municipio PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_codigo_municipio COMMAND tst_codigo_municipio)

add_executable(tst_cep tst_cep.cpp)
target_link_libraries(tst_cep PRIVATE Qt6::Test nl_back nl_warnings)
nl_registra_teste_qt(NAME tst_cep COMMAND tst_cep)
