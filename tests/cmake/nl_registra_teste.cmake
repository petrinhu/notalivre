# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Registro único dos testes Qt Test (NL-2.14, DA-2.61). Todo executável Qt Test entra no CTest por nl_registra_teste_qt, que
# acrescenta a saída junitxml de cada teste em <build>/junit/<NOME>.xml, além do texto no stdout (-o -,txt). O CTest conta
# um QSKIP como passou (o processo sai com 0), por isso tools/portoes/pulos_testes.sh lê esses arquivos, função a função.
# Testes que não são Qt Test (python, cmake, shell) continuam com add_test direto e ficam fora do manifesto.
#
# Uso:  nl_registra_teste_qt(NAME <nome> COMMAND <executavel> [argumentos...])
# Manifesto: <build>/junit/esperados.txt, gravado por nl_grava_manifesto_junit() no fim de tests/CMakeLists.txt.
# Executáveis Qt: <build>/junit/executaveis_qt.txt, com todo alvo EXECUTABLE que linka Qt6::Test (NL-2.14, revisão devops
# ALTA-1). O portão cruza essa lista com o CTest: um Qt Test que sai do registro único aparece como fora do manifesto.

set_property(GLOBAL PROPERTY NL_JUNIT_ESPERADOS "")
set_property(GLOBAL PROPERTY NL_EXECUTAVEIS_QT "")

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

# Anda a árvore de diretórios a partir de <diretorio> e guarda os executáveis que linkam Qt6::Test (alvo direto).
function(nl_coleta_executaveis_qt diretorio)
    get_property(alvos DIRECTORY "${diretorio}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(alvo IN LISTS alvos)
        get_target_property(tipo ${alvo} TYPE)
        if(tipo STREQUAL "EXECUTABLE")
            get_target_property(libs ${alvo} LINK_LIBRARIES)
            if(libs AND ("Qt6::Test" IN_LIST libs))
                set_property(GLOBAL APPEND PROPERTY NL_EXECUTAVEIS_QT ${alvo})
            endif()
        endif()
    endforeach()
    get_property(subdiretorios DIRECTORY "${diretorio}" PROPERTY SUBDIRECTORIES)
    foreach(sub IN LISTS subdiretorios)
        nl_coleta_executaveis_qt("${sub}")
    endforeach()
endfunction()

function(nl_grava_manifesto_junit)
    # O set_property "" acima deixa um elemento vazio na frente da lista (APPEND o mantém): sai aqui, senão o manifesto
    # ganha uma linha vazia e a contagem do portão fica uma acima (NL-2.14, achado da própria rodada de registro).
    get_property(esperados GLOBAL PROPERTY NL_JUNIT_ESPERADOS)
    list(FILTER esperados EXCLUDE REGEX "^$")
    get_property(executaveis GLOBAL PROPERTY NL_EXECUTAVEIS_QT)
    list(FILTER executaveis EXCLUDE REGEX "^$")
    if(NOT executaveis)
        message(FATAL_ERROR "nl_grava_manifesto_junit: nenhum executável Qt Test encontrado: a conferência do registro não teria o que cruzar")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/junit")
    list(JOIN esperados "\n" texto)
    file(WRITE "${CMAKE_BINARY_DIR}/junit/esperados.txt" "${texto}\n")
    list(JOIN executaveis "\n" texto_exec)
    file(WRITE "${CMAKE_BINARY_DIR}/junit/executaveis_qt.txt" "${texto_exec}\n")
endfunction()
