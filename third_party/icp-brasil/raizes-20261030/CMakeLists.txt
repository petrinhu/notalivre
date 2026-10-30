# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Raízes da ICP-Brasil de 2026-10-30 (NL-2.13, DA-2.47). Os .der estão na árvore; nada é baixado no build.
# O configure confere o SHA-256 de cada .der contra SHA256SUMS e aborta com os dois hashes se divergir.
# Exporta, como variáveis de cache INTERNAL: NL_ICP_RAIZES_DIR (diretório dos .der) e NL_ICP_RAIZES_MANIFESTO
# (o SHA256SUMS). O Flatpak instala os arquivos em share/notalivre/icp-brasil (CMAKE_INSTALL_PREFIX = /app).

set(_nl_icp_manifesto "${CMAKE_CURRENT_SOURCE_DIR}/SHA256SUMS")
if(NOT EXISTS "${_nl_icp_manifesto}")
    message(FATAL_ERROR "NL-2.13: manifesto de raízes ausente: ${_nl_icp_manifesto}. Sem ele o build não sabe o que conferir.")
endif()

# Linhas "<sha256>  <arquivo>" (formato do sha256sum). Piso de conferência: não-vazio (L-36).
file(STRINGS "${_nl_icp_manifesto}" _nl_icp_entradas REGEX "^[0-9a-f]+  [A-Za-z0-9_.-]+\\.der$")
list(LENGTH _nl_icp_entradas _nl_icp_n)
if(_nl_icp_n EQUAL 0)
    message(FATAL_ERROR "NL-2.13: ${_nl_icp_manifesto} sem nenhuma linha de hash. Piso de conferência: não-vazio.")
endif()

set(_nl_icp_arquivos "")
foreach(_nl_icp_linha IN LISTS _nl_icp_entradas)
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\1" _nl_icp_esperado "${_nl_icp_linha}")
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\2" _nl_icp_nome "${_nl_icp_linha}")
    set(_nl_icp_arq "${CMAKE_CURRENT_SOURCE_DIR}/${_nl_icp_nome}")
    if(NOT EXISTS "${_nl_icp_arq}")
        message(FATAL_ERROR "NL-2.13: ${_nl_icp_nome} listado em SHA256SUMS não existe em ${CMAKE_CURRENT_SOURCE_DIR}.")
    endif()
    file(SHA256 "${_nl_icp_arq}" _nl_icp_obtido)
    if(NOT _nl_icp_obtido STREQUAL _nl_icp_esperado)
        message(FATAL_ERROR "NL-2.13: SHA-256 divergente em ${_nl_icp_nome}: esperado ${_nl_icp_esperado}, obtido ${_nl_icp_obtido}. Não usar esta raiz.")
    endif()
    list(APPEND _nl_icp_arquivos "${_nl_icp_arq}")
endforeach()

# Dados, não código: os .der vão para share/notalivre/icp-brasil e nunca entram em nenhum alvo compilado.
install(FILES ${_nl_icp_arquivos} "${_nl_icp_manifesto}"
        DESTINATION share/notalivre/icp-brasil)

# Aviso de origem e licença (K2 do CLO, 30/10/2026, condição 2): instalado também no diretório de licenças do app.
set(_nl_icp_notice "${CMAKE_CURRENT_SOURCE_DIR}/NOTICE")
if(NOT EXISTS "${_nl_icp_notice}")
    message(FATAL_ERROR "NL-2.13: NOTICE das raízes da ICP-Brasil ausente: ${_nl_icp_notice}. O K2 exige o aviso junto dos arquivos.")
endif()
install(FILES "${_nl_icp_notice}"
        DESTINATION share/licenses/io.github.petrinhu.notalivre
        RENAME NOTICE-icp-brasil)

set(NL_ICP_RAIZES_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "Diretório com as raízes da ICP-Brasil (.der), conferidas")
set(NL_ICP_RAIZES_MANIFESTO "${_nl_icp_manifesto}" CACHE INTERNAL "SHA256SUMS das raízes da ICP-Brasil")
# SHA-256 do SHA256SUMS, embutido no binário (CISO, 30/10/2026, item 1): o conjunto de produção é o fixado, e a carga
# de produção recusa qualquer SHA256SUMS com outro hash antes de ler uma raiz.
file(SHA256 "${_nl_icp_manifesto}" _nl_icp_manifesto_sha)
set(NL_ICP_RAIZES_MANIFESTO_SHA256 "${_nl_icp_manifesto_sha}" CACHE INTERNAL "SHA-256 do SHA256SUMS das raízes, embutido no build")
message(STATUS "NL-2.13: raízes da ICP-Brasil conferidas (${_nl_icp_n} arquivos) em ${CMAKE_CURRENT_SOURCE_DIR}")
