# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de integração com servidor de banco (NL-3.1h, D-1 opção A). Só entram com NL_TESTES_BANCO=ON (preset banco), e
# nunca no build Flatpak (tests/CMakeLists.txt). Slots integracao.banco.<nome>, um por função, com o rótulo banco.
# Sem o cabeçalho ou a biblioteca do Connector/C a configuração para com mensagem: é erro de ambiente, não pulo.
# NL-3.1h (v3.17): o Connector/C do teste é o do prefixo declarado da distro (tools/conteiner/prefixo_mariadb.txt). A guarda
# está em src/foundation/sql/nl_conector_mariadb.cmake (NL-3.1a, v1f), a mesma do motor sql: o host passa NL_MARIADB_PREFIX ao contêiner.
include("${CMAKE_SOURCE_DIR}/src/foundation/sql/nl_conector_mariadb.cmake")

add_executable(tst_conexao_banco tst_conexao_banco.cpp)
target_include_directories(tst_conexao_banco PRIVATE ${NL_MARIADB_INCLUDE_DIR})
# G-R do CISO (NL-3.1h, v3.18): o teste afirma, na execução, que o libmariadb carregado mora neste prefixo.
target_compile_definitions(tst_conexao_banco PRIVATE "NL_MARIADB_PREFIXO_DO_BUILD=\"${NL_MARIADB_PREFIX}\"")
target_link_libraries(tst_conexao_banco PRIVATE Qt6::Core Qt6::Test ${NL_MARIADB_LIB} nl_warnings)

# Quatro conexões do critério 4 de TLS (C4', NL-3.1h): k = 4 slots, um por comportamento (sessão padrão, modo aberto,
# modo fixado com a impressão certa e modo fixado com a impressão trocada).
foreach(slot IN ITEMS sessaoPadrao modoAberto modoFixadoImpressaoCerta modoFixadoImpressaoTrocada)
  nl_registra_teste_qt(NAME integracao.banco.${slot} COMMAND tst_conexao_banco ${slot})
  set_tests_properties(integracao.banco.${slot} PROPERTIES
                       LABELS banco
                       TIMEOUT 120)  # teto explícito: um servidor travado não segura o par por 1500 s
endforeach()
