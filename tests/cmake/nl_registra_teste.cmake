# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Registro único dos testes Qt Test (NL-2.14, DA-2.61). Todo executável Qt Test entra no CTest por nl_registra_teste_qt, que
# acrescenta a saída junitxml de cada teste em <build>/junit/<NOME>.xml, além do texto no stdout (-o -,txt). O CTest conta
# um QSKIP como passou (o processo sai com 0), por isso tools/portoes/pulos_testes.sh lê esses arquivos, função a função.
# Testes que não são Qt Test (python, cmake, shell) continuam com add_test direto e ficam fora do manifesto.
#
# Uso:  nl_registra_teste_qt(NAME <nome> COMMAND <executavel> [argumentos...])
# O manifesto <build>/junit/esperados.txt é gravado por nl_grava_manifesto_junit() no fim de tests/CMakeLists.txt.

set_property(GLOBAL PROPERTY NL_JUNIT_ESPERADOS "")

function(nl_registra_teste_qt)
    cmake_parse_arguments(NL_TESTE "" "NAME" "COMMAND" ${ARGN})
    if(NOT NL_TESTE_NAME OR NOT NL_TESTE_COMMAND)
        message(FATAL_ERROR "nl_registra_teste_qt: NAME e COMMAND são obrigatórios")
    endif()
    list(POP_FRONT NL_TESTE_COMMAND NL_TESTE_EXECUTAVEL)
    set(NL_TESTE_XML "${CMAKE_BINARY_DIR}/junit/${NL_TESTE_NAME}.xml")
    add_test(NAME ${NL_TESTE_NAME}
             COMMAND ${NL_TESTE_EXECUTAVEL} -o -,txt -o ${NL_TESTE_XML},junitxml ${NL_TESTE_COMMAND})
    set_property(GLOBAL APPEND PROPERTY NL_JUNIT_ESPERADOS ${NL_TESTE_NAME})
endfunction()

function(nl_grava_manifesto_junit)
    get_property(esperados GLOBAL PROPERTY NL_JUNIT_ESPERADOS)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/junit")
    list(JOIN esperados "\n" texto)
    file(WRITE "${CMAKE_BINARY_DIR}/junit/esperados.txt" "${texto}\n")
endfunction()
