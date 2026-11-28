# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Tabelas do Anexo A v1.00 (municípios IBGE, países ISO2 e localidades), NL-3.M passo 1, opção 1 (decisão do líder, 28/11/2026).
# Os JSON NÃO entram na árvore: o configure baixa o XLSX (tools/ci/baixar_zip_xsd.cmake, gov.br e depois espelho), confere o
# SHA-256 lido de tools/tabelas/fontes.json (fonte única), converte com tools/tabelas/converter.py --so-anexo-a, compara os
# três JSON com SHA256SUMS (divergência é fatal) e instala em share/notalivre/tabelas.
# Exporta, como variáveis de cache INTERNAL: NL_TABELAS_DIR (JSON convertidos, na pasta de build), NL_TABELAS_MANIFESTO
# (SHA256SUMS da árvore) e NL_TABELAS_MANIFESTO_SHA256 (pino do SHA256SUMS, embutido pelo adaptador da Mid).
# Cache do XLSX: NL_TABELAS_XLSX_CACHE (ambiente ou -D); vazio, cai para NL_XSD_ZIP_CACHE (mesmo diretório de downloads do build).
# Sem rede: o XLSX pré-baixado no cache, com o SHA-256 certo, não vai à rede.
cmake_minimum_required(VERSION 3.25)

find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

set(NL_TABELAS_XLSX_CACHE "" CACHE PATH "Diretório com o XLSX do Anexo A já baixado (build sem rede). Vazio: usa NL_XSD_ZIP_CACHE.")
if(NOT NL_TABELAS_XLSX_CACHE AND NOT "$ENV{NL_TABELAS_XLSX_CACHE}" STREQUAL "")
    set(NL_TABELAS_XLSX_CACHE "$ENV{NL_TABELAS_XLSX_CACHE}")
endif()
if(NOT NL_TABELAS_XLSX_CACHE)
    if(NOT "${NL_XSD_ZIP_CACHE}" STREQUAL "")
        set(NL_TABELAS_XLSX_CACHE "${NL_XSD_ZIP_CACHE}")
    elseif(NOT "$ENV{NL_XSD_ZIP_CACHE}" STREQUAL "")
        set(NL_TABELAS_XLSX_CACHE "$ENV{NL_XSD_ZIP_CACHE}")
    endif()
endif()

# URL e âncora do XLSX: tools/tabelas/fontes.json é a fonte única (URL, arquivo e SHA-256 do anexo A).
set(_nl_tab_raiz "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(_nl_tab_fontes "${_nl_tab_raiz}/tools/tabelas/fontes.json")
set(_nl_tab_conversor "${_nl_tab_raiz}/tools/tabelas/converter.py")
set(_nl_tab_baixar "${_nl_tab_raiz}/tools/ci/baixar_zip_xsd.cmake")
set(_nl_tab_manifesto "${CMAKE_CURRENT_SOURCE_DIR}/SHA256SUMS")
if(NOT EXISTS "${_nl_tab_fontes}" OR NOT EXISTS "${_nl_tab_conversor}" OR NOT EXISTS "${_nl_tab_baixar}")
    message(FATAL_ERROR "NL-3.M: falta fontes.json, converter.py ou baixar_zip_xsd.cmake em tools/.")
endif()
if(NOT EXISTS "${_nl_tab_manifesto}")
    message(FATAL_ERROR "NL-3.M: manifesto de hashes ausente: ${_nl_tab_manifesto}.")
endif()

file(READ "${_nl_tab_fontes}" _nl_tab_json)
string(JSON _nl_tab_url GET "${_nl_tab_json}" anexos A url)
string(JSON _nl_tab_arquivo GET "${_nl_tab_json}" anexos A arquivo)
string(JSON _nl_tab_sha GET "${_nl_tab_json}" anexos A sha256)
string(LENGTH "${_nl_tab_sha}" _nl_tab_sha_len)
if(NOT _nl_tab_sha_len EQUAL 64 OR NOT _nl_tab_sha MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "NL-3.M: sha256 do anexo A em fontes.json inválido: '${_nl_tab_sha}'.")
endif()

# Espelho: branch de dados espelho-xsd, fixada pelo commit bde0e13 (decisão do líder, 28/11/2026). Vazio desliga.
set(NL_TABELAS_ESPELHO_URL "https://raw.githubusercontent.com/petrinhu/notalivre/bde0e131644364825cd3d4459e075812ac65b572/anexo-a-municipio-ibge-paises-iso2-snnfse-v1.00-20251210.xlsx" CACHE STRING "Espelho do XLSX do anexo A (branch espelho-xsd, commit fixo), tentado se o gov.br falhar. Vazio desliga.")
set(_nl_tab_urls "${_nl_tab_url}")
if(NOT NL_TABELAS_ESPELHO_URL STREQUAL "")
    list(APPEND _nl_tab_urls "${NL_TABELAS_ESPELHO_URL}")
endif()

set(_nl_tab_dir "${CMAKE_CURRENT_BINARY_DIR}/anexo-a")
set(_nl_tab_xlsx "${_nl_tab_dir}/${_nl_tab_arquivo}")
set(_nl_tab_saida "${_nl_tab_dir}/json")
file(MAKE_DIRECTORY "${_nl_tab_dir}")

# 1. XLSX: cache copiado antes (build sem rede); o baixar_zip_xsd.cmake não vai à rede se o arquivo já tem o SHA-256.
if(NOT NL_TABELAS_XLSX_CACHE STREQUAL "" AND EXISTS "${NL_TABELAS_XLSX_CACHE}/${_nl_tab_arquivo}")
    file(COPY_FILE "${NL_TABELAS_XLSX_CACHE}/${_nl_tab_arquivo}" "${_nl_tab_xlsx}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DNL_URLS=${_nl_tab_urls}" "-DNL_DESTINO=${_nl_tab_xlsx}" "-DNL_SHA256=${_nl_tab_sha}"
            -P "${_nl_tab_baixar}"
    RESULT_VARIABLE _nl_tab_rc
    OUTPUT_VARIABLE _nl_tab_out
    ERROR_VARIABLE _nl_tab_err)
if(NOT _nl_tab_rc EQUAL 0)
    file(REMOVE "${_nl_tab_xlsx}")
    message(FATAL_ERROR "NL-3.M: não consegui o XLSX do anexo A (URLs: ${_nl_tab_urls}, SHA-256 ${_nl_tab_sha}): ${_nl_tab_out}${_nl_tab_err}")
endif()

# 2. Conversão só do anexo A, em pasta nova (nada de saída antiga entra).
file(REMOVE_RECURSE "${_nl_tab_saida}")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -I -B "${_nl_tab_conversor}" --so-anexo-a
            --xlsx-dir "${_nl_tab_dir}" --saida "${_nl_tab_saida}"
    RESULT_VARIABLE _nl_tab_rc
    OUTPUT_VARIABLE _nl_tab_out
    ERROR_VARIABLE _nl_tab_err)
if(NOT _nl_tab_rc EQUAL 0)
    message(FATAL_ERROR "NL-3.M: converter.py do anexo A saiu com ${_nl_tab_rc}: ${_nl_tab_out}${_nl_tab_err}")
endif()

# 3. Conferência dos três JSON gerados contra SHA256SUMS versionado. O manifesto tem de listar exatamente 3 arquivos.
file(STRINGS "${_nl_tab_manifesto}" _nl_tab_linhas REGEX "^[0-9a-f]+  [A-Za-z0-9_.-]+$")
list(LENGTH _nl_tab_linhas _nl_tab_n)
if(NOT _nl_tab_n EQUAL 3)
    message(FATAL_ERROR "NL-3.M: ${_nl_tab_manifesto} precisa de exatamente 3 linhas de hash, achei ${_nl_tab_n}.")
endif()
set(_nl_tab_instalar "")
foreach(_nl_tab_linha IN LISTS _nl_tab_linhas)
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\1" _nl_tab_esperado "${_nl_tab_linha}")
    string(REGEX REPLACE "^([0-9a-f]+)  (.+)$" "\\2" _nl_tab_nome "${_nl_tab_linha}")
    set(_nl_tab_arq "${_nl_tab_saida}/${_nl_tab_nome}")
    if(NOT EXISTS "${_nl_tab_arq}")
        message(FATAL_ERROR "NL-3.M: ${_nl_tab_nome} listado em SHA256SUMS não foi gerado pelo conversor.")
    endif()
    file(SHA256 "${_nl_tab_arq}" _nl_tab_obtido)
    if(NOT _nl_tab_obtido STREQUAL _nl_tab_esperado)
        message(FATAL_ERROR "NL-3.M: SHA-256 divergente em ${_nl_tab_nome}: esperado ${_nl_tab_esperado}, obtido ${_nl_tab_obtido}. Não usar estas tabelas.")
    endif()
    list(APPEND _nl_tab_instalar "${_nl_tab_arq}")
endforeach()

# 3b. O SHA256SUMS da árvore vai para a pasta de saída, byte a byte, ao lado dos JSON: NL_TABELAS_DIR fica com os quatro
# arquivos, como o pacote instalado. O pino abaixo continua sendo o do arquivo da árvore (igual à cópia).
file(COPY_FILE "${_nl_tab_manifesto}" "${_nl_tab_saida}/SHA256SUMS")

# 4. Instalação: dados, não código (K2). O SHA256SUMS vai junto, para o adaptador conferir de novo na execução.
install(FILES ${_nl_tab_instalar} "${_nl_tab_manifesto}"
        DESTINATION share/notalivre/tabelas)

# Pino do SHA256SUMS, embutido na Mid (como o do XSD, NL-2.10b).
file(SHA256 "${_nl_tab_manifesto}" _nl_tab_manifesto_sha)
string(LENGTH "${_nl_tab_manifesto_sha}" _nl_tab_manifesto_sha_len)
if(NOT _nl_tab_manifesto_sha_len EQUAL 64)
    message(FATAL_ERROR "NL-3.M: SHA-256 do SHA256SUMS das tabelas com ${_nl_tab_manifesto_sha_len} caracteres, esperado 64.")
endif()

set(NL_TABELAS_DIR "${_nl_tab_saida}" CACHE INTERNAL "Tabelas do anexo A convertidas e conferidas")
set(NL_TABELAS_MANIFESTO "${_nl_tab_manifesto}" CACHE INTERNAL "SHA256SUMS das tabelas do anexo A")
set(NL_TABELAS_MANIFESTO_SHA256 "${_nl_tab_manifesto_sha}" CACHE INTERNAL "SHA-256 do SHA256SUMS das tabelas, embutido na Mid")
message(STATUS "NL-3.M: tabelas do anexo A conferidas (3 JSON, XLSX ${_nl_tab_sha}) em ${_nl_tab_saida}")
