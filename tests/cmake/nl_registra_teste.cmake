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
    nl_gate_sanitizer_registra(${NL_TESTE_NAME})
endfunction()

# Gate de sanitizer (NL-3.0h, decisão do main). Um processo de teste que sai com erro do LeakSanitizer ou do
# AddressSanitizer tem de reprovar o teste. O CTest ignora o código de saída quando PASS_REGULAR_EXPRESSION casa (medido:
# o processo sai com 1 e o teste fica Passed). Por isso o FAIL de sanitizer é acrescentado a todo teste que tem PASS,
# por um ponto único: nl_gate_sanitizer_fecha(), chamado no topo depois de add_subdirectory(tests).
# O FAIL vence o PASS quando os dois casam (medido nas quatro imagens). Cada teste que não sai pelo nl_registra_teste_qt
# se registra com nl_gate_sanitizer_registra(NAME).
function(nl_gate_sanitizer_registra nome)
    set_property(GLOBAL APPEND PROPERTY NL_GATE_TESTES "${nome}|${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

function(nl_gate_sanitizer_fecha)
    get_property(registrados GLOBAL PROPERTY NL_GATE_TESTES)
    foreach(entrada IN LISTS registrados)
        string(REPLACE "|" ";" partes "${entrada}")
        list(GET partes 0 nome)
        list(GET partes 1 diretorio)
        get_property(comPass TEST ${nome} DIRECTORY ${diretorio} PROPERTY PASS_REGULAR_EXPRESSION SET)
        if(comPass)
            set_property(TEST ${nome} DIRECTORY ${diretorio} APPEND PROPERTY
                         FAIL_REGULAR_EXPRESSION "ERROR: (Leak|Address)Sanitizer" "runtime error:")
        endif()
    endforeach()
endfunction()
