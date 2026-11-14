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

# Gate de sanitizer (NL-3.0h; NL-3.A, condição C4.1 do CISO, forma (a)). O CTest ignora o código de saída quando
# PASS_REGULAR_EXPRESSION casa, e por isso nenhum teste usa regex de passe (portão estático tools/portoes/codigo_de_saida.py):
# o código de saída decide, e o do sanitizer é 86. O texto do LeakSanitizer, do AddressSanitizer e do UBSan reprova também,
# como segunda defesa, TODO teste do CTest: nl_gate_sanitizer_fecha(), chamado no topo depois de add_subdirectory(tests),
# percorre a árvore de diretórios (propriedades SUBDIRECTORIES e TESTS) e põe os dois regex em cada teste, sem registro
# manual e sem isentos. O regex não exige "ERROR:", porque "LeakSanitizer has encountered a fatal error" sai sem isso.
# tools/portoes/sanitizer_ativo.py confere estes dois regex, por igualdade, em todo teste.
function(nl_gate_sanitizer_fecha)
    nl_gate_sanitizer_aplica("${CMAKE_SOURCE_DIR}")
endfunction()

function(nl_gate_sanitizer_aplica diretorio)
    get_property(testes DIRECTORY "${diretorio}" PROPERTY TESTS)
    foreach(nome IN LISTS testes)
        set_property(TEST "${nome}" DIRECTORY "${diretorio}" APPEND PROPERTY
                     FAIL_REGULAR_EXPRESSION "(Leak|Address)Sanitizer" "runtime error:")
    endforeach()
    get_property(filhos DIRECTORY "${diretorio}" PROPERTY SUBDIRECTORIES)
    foreach(filho IN LISTS filhos)
        nl_gate_sanitizer_aplica("${filho}")
    endforeach()
endfunction()
