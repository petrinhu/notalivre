# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de integração com servidor de banco (NL-3.1h, D-1 opção A). Só entram com NL_TESTES_BANCO=ON (preset banco), e
# nunca no build Flatpak (tests/CMakeLists.txt). Slots integracao.banco.<nome>, um por função, com o rótulo banco.
# Sem o cabeçalho ou a biblioteca do Connector/C a configuração para com mensagem: é erro de ambiente, não pulo.
# NL-3.1h (v3.17): o Connector/C do teste é o do prefixo declarado da distro (tools/conteiner/prefixo_mariadb.txt), que o
# executar_no_conteiner.sh exporta em NL_MARIADB_PREFIX. Sem prefixo, a configuração para: falha fechada.
set(NL_MARIADB_PREFIX "$ENV{NL_MARIADB_PREFIX}" CACHE PATH "Prefixo do Connector/C: cabeçalho e biblioteca da mesma instalação")
if(NOT NL_MARIADB_PREFIX)
    message(FATAL_ERROR "tests/integracao/banco: NL_MARIADB_PREFIX vazio; o prefixo vem de tools/conteiner/prefixo_mariadb.txt (NL-3.1h, v3.17)")
endif()
find_path(NL_MARIADB_INCLUDE_DIR NAMES mariadb/mysql.h)
find_library(NL_MARIADB_LIB NAMES mariadb)
if(NOT NL_MARIADB_INCLUDE_DIR OR NOT NL_MARIADB_LIB)
    message(FATAL_ERROR "tests/integracao/banco: mariadb/mysql.h ou libmariadb ausente (NL-3.1h, passo 6.4)")
endif()
# Guarda (NL-3.1h, v3.17): cabeçalho e biblioteca do mesmo prefixo. Em 742c100 o cabeçalho era o do módulo (/usr/local) e a
# biblioteca a da distro (/usr/lib64), achados cada um pela própria ordem de busca.
foreach(_nl_achado IN ITEMS "${NL_MARIADB_INCLUDE_DIR}" "${NL_MARIADB_LIB}")
    string(FIND "${_nl_achado}" "${NL_MARIADB_PREFIX}/" _nl_pos)
    if(NOT _nl_pos EQUAL 0)
        message(FATAL_ERROR "tests/integracao/banco: ${_nl_achado} fora do prefixo ${NL_MARIADB_PREFIX}: cabeçalho e biblioteca de instalações diferentes (NL-3.1h, v3.17)")
    endif()
endforeach()

add_executable(tst_conexao_banco tst_conexao_banco.cpp)
target_include_directories(tst_conexao_banco PRIVATE ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(tst_conexao_banco PRIVATE Qt6::Core Qt6::Test ${NL_MARIADB_LIB} nl_warnings)

# Quatro conexões do critério 4 de TLS (C4', NL-3.1h): k = 4 slots, um por comportamento (sessão padrão, modo aberto,
# modo fixado com a impressão certa e modo fixado com a impressão trocada).
foreach(slot IN ITEMS sessaoPadrao modoAberto modoFixadoImpressaoCerta modoFixadoImpressaoTrocada)
  nl_registra_teste_qt(NAME integracao.banco.${slot} COMMAND tst_conexao_banco ${slot})
  set_tests_properties(integracao.banco.${slot} PROPERTIES
                       LABELS banco
                       TIMEOUT 120)  # teto explícito: um servidor travado não segura o par por 1500 s
endforeach()
