# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Função de registro dos testes Qt Test (NL-2.14, DA-2.61 adendo 3). Todo teste Qt Test entra no CTest por
# nl_registra_teste_qt, que injeta a saída junitxml por função em <build>/junit/<NOME>.xml, além do texto no stdout
# (-o -,txt). O CTest conta um QSKIP como passou (o processo sai com 0), e o portão tools/portoes/pulos_testes.sh lê esses
# arquivos. A lista de testes não vem daqui: vem de ctest --show-only=json-v1, e o portão exige junitxml de todo teste que
# não esteja em tests/testes_sem_saida_por_funcao.txt. Um teste que não passa por esta função não gera junitxml e reprova.
#
# Uso:  nl_registra_teste_qt(NAME <nome> COMMAND <executavel> [argumentos...])

function(nl_registra_teste_qt)
    cmake_parse_arguments(NL_TESTE "" "NAME" "COMMAND" ${ARGN})
    if(NOT NL_TESTE_NAME OR NOT NL_TESTE_COMMAND)
        message(FATAL_ERROR "nl_registra_teste_qt: NAME e COMMAND são obrigatórios")
    endif()
    list(POP_FRONT NL_TESTE_COMMAND NL_TESTE_EXECUTAVEL)
    set(NL_TESTE_XML "${CMAKE_BINARY_DIR}/junit/${NL_TESTE_NAME}.xml")
    add_test(NAME ${NL_TESTE_NAME}
             COMMAND ${NL_TESTE_EXECUTAVEL} -o -,txt -o ${NL_TESTE_XML},junitxml ${NL_TESTE_COMMAND})
endfunction()
