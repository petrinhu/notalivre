# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# XSD de Produção Restrita, leiaute de 20/10/2026 (NL-1.6a). Os XSD NÃO entram na árvore: o configure baixa o ZIP
# de URL fixa para o diretório de build, confere o SHA-256 do ZIP e de cada .xsd contra SHA256SUMS e extrai.
# Qualquer divergência aborta o configure (FATAL_ERROR com o nome do arquivo e os dois hashes).
# Sem rede: NL_XSD_ZIP_CACHE aponta para um diretório que contém o ZIP pré-baixado; o hash continua conferido.
# Exporta, como variáveis de cache INTERNAL: NL_XSD_PRODREST_DIR (árvore extraída), NL_XSD_PRODREST_MANIFESTO
# (SHA256SUMS da árvore de código) e NL_XSD_PRODREST_RAIZ (DPS_v1.01.xsd).

set(NL_XSD_ZIP_CACHE "" CACHE PATH "Diretório com o ZIP do XSD já baixado (build sem rede). O SHA-256 é conferido do mesmo jeito.")
# URL do ZIP: o padrão é o gov.br; o teste de hash (tests/unit/foundation/xml/hash_zip) aponta file:// para ficar sem rede.
set(NL_XSD_URL "https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita/esquemas_prod_restrita_20261020.zip" CACHE STRING "URL do ZIP do XSD de Produção Restrita")
set(_nl_xsd_url "${NL_XSD_URL}")
set(_nl_xsd_zip_nome "esquemas_prod_restrita_20261020.zip")
set(_nl_xsd_manifesto "${CMAKE_CURRENT_SOURCE_DIR}/SHA256SUMS")
set(_nl_xsd_dir "${CMAKE_CURRENT_BINARY_DIR}/nfse-xsd")
set(_nl_xsd_zip "${_nl_xsd_dir}/${_nl_xsd_zip_nome}")
set(_nl_xsd_extraido "${_nl_xsd_dir}/extraido")

if(NOT EXISTS "${_nl_xsd_manifesto}")
    message(FATAL_ERROR "NL-1.6a: manifesto de hashes ausente: ${_nl_xsd_manifesto}. Sem ele o build não sabe o que conferir.")
endif()

# Uma linha "# ZIP SHA-256: <64 hex> (...)" traz o hash do ZIP inteiro.
file(STRINGS "${_nl_xsd_manifesto}" _nl_xsd_linhas_zip REGEX "^# ZIP SHA-256: [0-9a-f]+")
list(LENGTH _nl_xsd_linhas_zip _nl_xsd_n_zip)
if(NOT _nl_xsd_n_zip EQUAL 1)
    message(FATAL_ERROR "NL-1.6a: ${_nl_xsd_manifesto} precisa de exatamente uma linha '# ZIP SHA-256:', achei ${_nl_xsd_n_zip}.")
endif()
string(REGEX REPLACE "^# ZIP SHA-256: ([0-9a-f]+).*$" "\\1" _nl_xsd_zip_sha "${_nl_xsd_linhas_zip}")
string(LENGTH "${_nl_xsd_zip_sha}" _nl_xsd_zip_sha_len)
if(NOT _nl_xsd_zip_sha_len EQUAL 64)
    message(FATAL_ERROR "NL-1.6a: SHA-256 do ZIP com ${_nl_xsd_zip_sha_len} caracteres em ${_nl_xsd_manifesto}, esperado 64.")
endif()

# Linhas "<sha256>  <arquivo>" no formato do sha256sum; só os .xsd entram aqui.
file(STRINGS "${_nl_xsd_manifesto}" _nl_xsd_entradas REGEX "^[0-9a-f]+  [A-Za-z0-9_.-]+$")
list(LENGTH _nl_xsd_entradas _nl_xsd_n_entradas)
if(_nl_xsd_n_entradas EQUAL 0)
    message(FATAL_ERROR "NL-1.6a: ${_nl_xsd_manifesto} sem nenhuma linha de hash. Piso de conferência: não-vazio.")
endif()

# 1. ZIP: cache local opcional, depois download com EXPECTED_HASH (pula o download se o arquivo já tem o hash).
if(NL_XSD_ZIP_CACHE AND EXISTS "${NL_XSD_ZIP_CACHE}/${_nl_xsd_zip_nome}")
    file(MAKE_DIRECTORY "${_nl_xsd_dir}")
    file(COPY_FILE "${NL_XSD_ZIP_CACHE}/${_nl_xsd_zip_nome}" "${_nl_xsd_zip}")
endif()
file(MAKE_DIRECTORY "${_nl_xsd_dir}")
file(DOWNLOAD "${_nl_xsd_url}" "${_nl_xsd_zip}"
    EXPECTED_HASH SHA256=${_nl_xsd_zip_sha}
    TLS_VERIFY ON
    TIMEOUT 120
    STATUS _nl_xsd_status)
list(GET _nl_xsd_status 0 _nl_xsd_cod)
if(NOT _nl_xsd_cod EQUAL 0)
    list(GET _nl_xsd_status 1 _nl_xsd_msg)
    message(FATAL_ERROR "NL-1.6a: não consegui obter o ZIP do XSD de ${_nl_xsd_url} com SHA-256 esperado ${_nl_xsd_zip_sha}: ${_nl_xsd_msg}. "
        "Sem rede no build: pré-baixe o ZIP e aponte NL_XSD_ZIP_CACHE para o diretório dele.")
endif()

# 2. Extração em diretório de build (nunca na árvore de código).
file(REMOVE_RECURSE "${_nl_xsd_extraido}")
file(ARCHIVE_EXTRACT INPUT "${_nl_xsd_zip}" DESTINATION "${_nl_xsd_extraido}")

# 3. Conferência de cada arquivo do manifesto contra o extraído.
foreach(_nl_xsd_linha IN LISTS _nl_xsd_entradas)
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\1" _nl_xsd_esperado "${_nl_xsd_linha}")
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\2" _nl_xsd_nome "${_nl_xsd_linha}")
    set(_nl_xsd_arq "${_nl_xsd_extraido}/${_nl_xsd_nome}")
    if(NOT EXISTS "${_nl_xsd_arq}")
        message(FATAL_ERROR "NL-1.6a: ${_nl_xsd_nome} listado em SHA256SUMS não existe no ZIP extraído.")
    endif()
    file(SHA256 "${_nl_xsd_arq}" _nl_xsd_obtido)
    if(NOT _nl_xsd_obtido STREQUAL _nl_xsd_esperado)
        message(FATAL_ERROR "NL-1.6a: SHA-256 divergente em ${_nl_xsd_nome}: esperado ${_nl_xsd_esperado}, obtido ${_nl_xsd_obtido}. Não usar este XSD.")
    endif()
endforeach()

set(NL_XSD_PRODREST_DIR "${_nl_xsd_extraido}" CACHE INTERNAL "XSD de Produção Restrita 20/10/2026, extraído e conferido")
set(NL_XSD_PRODREST_MANIFESTO "${_nl_xsd_manifesto}" CACHE INTERNAL "SHA256SUMS do leiaute de 20/10/2026")
set(NL_XSD_PRODREST_RAIZ "DPS_v1.01.xsd" CACHE INTERNAL "Esquema raiz da DPS")
set(NL_XSD_PRODREST_ZIP "${_nl_xsd_zip}" CACHE INTERNAL "ZIP baixado e conferido (usado pelo teste de hash)")
message(STATUS "NL-1.6a: XSD conferidos (${_nl_xsd_n_entradas} arquivos, ZIP ${_nl_xsd_zip_sha}) em ${_nl_xsd_extraido}")
