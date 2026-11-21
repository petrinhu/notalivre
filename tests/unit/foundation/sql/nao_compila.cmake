# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Teste de não-compilação da Foundation sql (NL-3.1a, bloco U-R, CONTRACT 12.1). O negativo tem de FALHAR ao compilar e o
# positivo tem de COMPILAR. Os dois arquivos do par diferem só numa linha, a que tem a marca de caso. Sem junitxml por
# função: o teste sai pelo código de saída, e o portão de testes sem saída lista o nome.
# Entradas pelo ENVIRONMENT do teste (sem argumentos depois do script): NL_CXX, NL_INCLUDES (separados por ":"),
# NL_NEGATIVO e NL_POSITIVO.

foreach(variavel IN ITEMS NL_CXX NL_INCLUDES NL_NEGATIVO NL_POSITIVO)
    if("$ENV{${variavel}}" STREQUAL "")
        message(FATAL_ERROR "nao_compila: ${variavel} ausente no ENVIRONMENT do teste")
    endif()
endforeach()
set(CXX "$ENV{NL_CXX}")
set(NEGATIVO "$ENV{NL_NEGATIVO}")
set(POSITIVO "$ENV{NL_POSITIVO}")
string(REPLACE ":" ";" INCLUDES "$ENV{NL_INCLUDES}")

foreach(arquivo IN ITEMS "${NEGATIVO}" "${POSITIVO}")
    file(STRINGS "${arquivo}" marcas REGEX "NL-CASO")
    list(LENGTH marcas quantas)
    if(NOT quantas EQUAL 1)
        message(FATAL_ERROR "nao_compila: ${arquivo} tem ${quantas} linhas de caso, e tem de ter 1")
    endif()
endforeach()

set(opcoes -std=c++23 -fsyntax-only -fPIC)
foreach(diretorio IN LISTS INCLUDES)
    list(APPEND opcoes "-I${diretorio}")
endforeach()

execute_process(COMMAND "${CXX}" ${opcoes} "${NEGATIVO}"
                RESULT_VARIABLE rc_negativo
                OUTPUT_QUIET ERROR_QUIET)
execute_process(COMMAND "${CXX}" ${opcoes} "${POSITIVO}"
                RESULT_VARIABLE rc_positivo
                OUTPUT_QUIET ERROR_VARIABLE saida_positivo)

if(rc_negativo EQUAL 0)
    message(FATAL_ERROR "nao_compila: o negativo COMPILOU (${NEGATIVO}): a regra não vale")
endif()
if(NOT rc_positivo EQUAL 0)
    message(FATAL_ERROR "nao_compila: o positivo NÃO compilou (${POSITIVO}):\n${saida_positivo}")
endif()
message(STATUS "nao_compila: negativo recusado e positivo aceito")
