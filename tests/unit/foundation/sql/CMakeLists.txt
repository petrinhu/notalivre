# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes de unidade da Foundation sql (NL-3.1a, bloco U-R): sem servidor, em todas as imagens. Os dois executáveis Qt Test
# entram por nl_registra_teste_qt. Os oito casos de não-compilação são scripts cmake -P (sem junitxml por função), que
# entram em tests/testes_sem_saida_por_funcao.txt e em flatpak/testes_fora_do_flatpak.txt.
include("${CMAKE_SOURCE_DIR}/src/foundation/sql/nl_conector_mariadb.cmake")

add_executable(tst_sql_validacao tst_sql_validacao.cpp)
target_include_directories(tst_sql_validacao PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado)
target_link_libraries(tst_sql_validacao PRIVATE Qt6::Test nl_foundation_sql nl_foundation_sql_costura nl_warnings)
nl_registra_teste_qt(NAME tst_sql_validacao COMMAND tst_sql_validacao)

add_executable(tst_sql_vinculos tst_sql_vinculos.cpp)
target_include_directories(tst_sql_vinculos PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(tst_sql_vinculos PRIVATE Qt6::Test nl_foundation_sql nl_warnings)
nl_registra_teste_qt(NAME tst_sql_vinculos COMMAND tst_sql_vinculos)

# Opções de TLS lidas de volta, sem servidor (NL-3.1k; parecer do CISO, R3): chama os átomos de modo do motor.
add_executable(tst_sql_opcoes_tls tst_sql_opcoes_tls.cpp)
target_include_directories(tst_sql_opcoes_tls PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado ${NL_MARIADB_INCLUDE_DIR})
target_link_libraries(tst_sql_opcoes_tls PRIVATE Qt6::Test nl_foundation_sql ${NL_MARIADB_LIB} nl_warnings)
nl_registra_teste_qt(NAME tst_sql_opcoes_tls COMMAND tst_sql_opcoes_tls)

# Não-compilação: o caso negativo tem de falhar ao compilar, e o positivo tem de compilar. Sem nl_warnings: um aviso não
# pode contar como falha de compilação. O comando é cmake -P <script absoluto> (formato exigido pelo portão pulos_testes);
# o compilador e os includes entram pelo ENVIRONMENT do teste. Os includes vão separados por ":" (caminhos não têm ":").
set(NL_CASOS_NAO_COMPILA
    consultaDeArrayLocal consultaConcatenada consultaComPontoEVirgula consultaComAspas
    consultaComComentario ddlNoLugarDeConsulta parametroQString dadosSemModo)
get_target_property(NL_INCLUDES_QT_CORE Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
list(FILTER NL_INCLUDES_QT_CORE EXCLUDE REGEX "\\$<")
set(NL_INCLUDES_NAO_COMPILA ${NL_INCLUDES_QT_CORE}
    ${CMAKE_SOURCE_DIR}/src/foundation/base/include
    ${CMAKE_SOURCE_DIR}/src/foundation/sql/include)
string(REPLACE ";" ":" NL_INCLUDES_NAO_COMPILA_TEXTO "${NL_INCLUDES_NAO_COMPILA}")
if(NOT NL_FLATPAK_BUILD)
foreach(caso IN LISTS NL_CASOS_NAO_COMPILA)
    add_test(NAME sql.naoCompila.${caso}
             COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/nao_compila.cmake")
    set_tests_properties(sql.naoCompila.${caso} PROPERTIES ENVIRONMENT
        "NL_CXX=${CMAKE_CXX_COMPILER};NL_INCLUDES=${NL_INCLUDES_NAO_COMPILA_TEXTO};NL_NEGATIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/${caso}_negativo.cpp;NL_POSITIVO=${CMAKE_CURRENT_SOURCE_DIR}/nao_compila/${caso}_positivo.cpp")
endforeach()
endif()

# Catálogo e plano do migrador, sem servidor (NL-3.1b, parte b1).
add_executable(tst_migracao tst_migracao.cpp)
target_link_libraries(tst_migracao PRIVATE Qt6::Test nl_foundation_sql nl_warnings)
nl_registra_teste_qt(NAME tst_migracao COMMAND tst_migracao)

# Resposta do GET_LOCK do migrador, sem servidor (NL-3.1b, parte b2; parecer do CISO, I-a).
add_executable(tst_migracao_trava tst_migracao_trava.cpp)
target_include_directories(tst_migracao_trava PRIVATE ${CMAKE_SOURCE_DIR}/src/foundation/sql/privado)
target_link_libraries(tst_migracao_trava PRIVATE Qt6::Test nl_foundation_sql nl_warnings)
nl_registra_teste_qt(NAME tst_migracao_trava COMMAND tst_migracao_trava)
