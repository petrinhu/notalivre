# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de unidade da Foundation sql (NL-3.1a, bloco U-R): sem servidor, em todas as imagens. Os dois executáveis Qt Test
# entram por nl_registra_teste_qt. Os oito casos de não-compilação são scripts cmake -P (sem junitxml por função), que
# entram em tests/testes_sem_saida_por_funcao.txt e em flatpak/testes_fora_do_flatpak.txt.
include("${CMAKE_SOURCE_DIR}/src/foundation/sql/nl_conector_mariadb.cmake")

add_executable(tst_sql_validacao tst_sql_validacao.cpp)
target_include_directories(tst_sql_validacao PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado)
target_link_libraries(tst_sql_validacao PRIVATE Qt6::Test nl_foundation_sql nl_warnings)
nl_registra_teste_qt(NAME tst_sql_validacao COMMAND tst_sql_validacao)

add_executable(tst_sql_vinculos tst_sql_vinculos.cpp)
target_include_directories(tst_sql_vinculos PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(tst_sql_vinculos PRIVATE Qt6::Test nl_foundation_sql nl_warnings)
nl_registra_teste_qt(NAME tst_sql_vinculos COMMAND tst_sql_vinculos)

# Não-compilação: o caso negativo tem de falhar ao compilar, e o positivo tem de compilar. Sem nl_warnings: um aviso não
# pode contar como falha de compilação. Os includes são os do motor e os do Qt Core, lidos na configuração.
set(NL_CASOS_NAO_COMPILA
    consultaDeArrayLocal consultaConcatenada consultaComPontoEVirgula consultaComAspas
    consultaComComentario ddlNoLugarDeConsulta parametroQString dadosSemModo)
get_target_property(NL_INCLUDES_QT_CORE Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
list(FILTER NL_INCLUDES_QT_CORE EXCLUDE REGEX "\\$<")
set(NL_INCLUDES_NAO_COMPILA ${NL_INCLUDES_QT_CORE}
    ${CMAKE_SOURCE_DIR}/src/foundation/base/include
    ${CMAKE_SOURCE_DIR}/src/foundation/sql/include)
if(NOT NL_FLATPAK_BUILD)
foreach(caso IN LISTS NL_CASOS_NAO_COMPILA)
    add_test(NAME sql.naoCompila.${caso}
             COMMAND ${CMAKE_COMMAND}
                     "-DCXX=${CMAKE_CXX_COMPILER}"
                     "-DINCLUDES=${NL_INCLUDES_NAO_COMPILA}"
                     "-DNEGATIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/${caso}_negativo.cpp"
                     "-DPOSITIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/${caso}_positivo.cpp"
                     -P "${CMAKE_CURRENT_SOURCE_DIR}/nao_compila.cmake")
endforeach()
endif()
