# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes unitários do Id da DPS, série, número e chave de acesso mínima (NL-1.3). Regra pura, sem I/O.
# Um executável por tipo, cada um no CTest pelo próprio nome.
add_executable(tst_serie tst_serie.cpp)
target_link_libraries(tst_serie PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_serie COMMAND tst_serie)

add_executable(tst_numero_dps tst_numero_dps.cpp)
target_link_libraries(tst_numero_dps PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_numero_dps COMMAND tst_numero_dps)

add_executable(tst_chave_acesso tst_chave_acesso.cpp)
target_link_libraries(tst_chave_acesso PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_chave_acesso COMMAND tst_chave_acesso)

add_executable(tst_id_dps tst_id_dps.cpp)
target_link_libraries(tst_id_dps PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_id_dps COMMAND tst_id_dps)

# NL-1.4: entidade Dps, regime, grupo IBS/CBS e tipos de leiaute (um executável por suíte, ajudantes.h compartilhado).
add_executable(tst_codigos_leiaute tst_codigos_leiaute.cpp)
target_link_libraries(tst_codigos_leiaute PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_codigos_leiaute COMMAND tst_codigos_leiaute)

add_executable(tst_descricao_servico tst_descricao_servico.cpp)
target_link_libraries(tst_descricao_servico PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_descricao_servico COMMAND tst_descricao_servico)

add_executable(tst_regime tst_regime.cpp)
target_link_libraries(tst_regime PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_regime COMMAND tst_regime)

add_executable(tst_pessoas tst_pessoas.cpp)
target_link_libraries(tst_pessoas PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_pessoas COMMAND tst_pessoas)

add_executable(tst_endereco tst_endereco.cpp)
target_link_libraries(tst_endereco PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_endereco COMMAND tst_endereco)

add_executable(tst_substituicao tst_substituicao.cpp)
target_link_libraries(tst_substituicao PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_substituicao COMMAND tst_substituicao)

add_executable(tst_ibscbs tst_ibscbs.cpp)
target_link_libraries(tst_ibscbs PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_ibscbs COMMAND tst_ibscbs)

add_executable(tst_tributacao tst_tributacao.cpp)
target_link_libraries(tst_tributacao PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_tributacao COMMAND tst_tributacao)

add_executable(tst_dps tst_dps.cpp)
target_link_libraries(tst_dps PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_dps COMMAND tst_dps)

# DA-1.27: valor dhEmi (corte de milissegundos, offset de hora cheia de -11:00 a +12:00, ano de 2000 a 2099).
add_executable(tst_dh_emi tst_dh_emi.cpp)
target_link_libraries(tst_dh_emi PRIVATE Qt6::Test nl_back nl_warnings)
add_test(NAME tst_dh_emi COMMAND tst_dh_emi)

