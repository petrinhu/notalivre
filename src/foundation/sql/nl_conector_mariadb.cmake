# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Connector/C do NotaLivre: prefixo único para o motor sql (src/foundation/sql) e para os testes de banco
# (tests/integracao/banco). Os dois usam esta guarda, a mesma regra e o mesmo átomo (NL-I.341, NL-3.1a).
# O prefixo vem da variável de ambiente NL_MARIADB_PREFIX, que o host passa ao contêiner (build_local.sh, a partir de
# tools/conteiner/prefixo_mariadb.txt) e que o CI exporta pelo $GITHUB_ENV. Sem prefixo, a configuração para: falha fechada.
# As mensagens dizem "tests/integracao/banco" por herança do texto da NL-3.1h; não mudam (NL-3.1a, emenda v1, bloco B).
set(NL_MARIADB_PREFIX "$ENV{NL_MARIADB_PREFIX}" CACHE PATH "Prefixo do Connector/C: cabeçalho e biblioteca da mesma instalação")
if(NOT NL_MARIADB_PREFIX)
    message(FATAL_ERROR "tests/integracao/banco: NL_MARIADB_PREFIX vazio; o prefixo vem de tools/conteiner/prefixo_mariadb.txt (NL-3.1h, v3.17)")
endif()
find_path(NL_MARIADB_INCLUDE_DIR NAMES mysql.h
          PATHS "${NL_MARIADB_PREFIX}/include/mariadb" "${NL_MARIADB_PREFIX}/include/mysql" NO_DEFAULT_PATH)
find_library(NL_MARIADB_LIB NAMES mariadb
             PATHS "${NL_MARIADB_PREFIX}/lib/mariadb" "${NL_MARIADB_PREFIX}/lib64" "${NL_MARIADB_PREFIX}/lib" NO_DEFAULT_PATH)
if(NOT NL_MARIADB_INCLUDE_DIR OR NOT NL_MARIADB_LIB)
    message(FATAL_ERROR "tests/integracao/banco: mysql.h ou libmariadb ausente em ${NL_MARIADB_PREFIX} (NL-3.1h, passo 6.4 e v3.17)")
endif()
# Guarda (NL-3.1h, v3.17): cabeçalho e biblioteca do mesmo prefixo. Em 742c100 o cabeçalho era o do módulo (/usr/local) e a
# biblioteca a da distro (/usr/lib64), achados cada um pela própria ordem de busca.
foreach(_nl_achado IN ITEMS "${NL_MARIADB_INCLUDE_DIR}" "${NL_MARIADB_LIB}")
    string(FIND "${_nl_achado}" "${NL_MARIADB_PREFIX}/" _nl_pos)
    if(NOT _nl_pos EQUAL 0)
        message(FATAL_ERROR "tests/integracao/banco: ${_nl_achado} fora do prefixo ${NL_MARIADB_PREFIX}: cabeçalho e biblioteca de instalações diferentes (NL-3.1h, v3.17)")
    endif()
endforeach()
