# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Download do ZIP do XSD de Produção Restrita no CI (NL-1.6c). Roda com `cmake -P`, presente nas quatro imagens
# de build. O cliente é o do CMake (file(DOWNLOAD)): o gov.br recusou o urllib do Python com 403 (run 40268530187).
# Uso: cmake -DNL_URLS=<url1;url2> -DNL_DESTINO=<arquivo> -DNL_SHA256=<64 hex minúsculos> -P tools/ci/baixar_zip_xsd.cmake
# NL_URLS é uma lista ordenada: a primeira é a origem (gov.br), a seguinte é o espelho (decisão do líder, 27/11/2026,
# "Branch de dados, sem release (Recomendado)"; run 46007980323, o gov.br recusou os runners com SSL connect error).
# Para cada URL: até 5 tentativas em qualquer falha de download (transporte ou HTTP). Passa à URL seguinte só se as 5
# falharem. O download é feito sem EXPECTED_HASH: com ele, o CMake aborta na hora em resposta HTTP de erro, sem tentar
# a seguinte. O SHA-256 é conferido depois, com file(SHA256). Divergência é fatal na hora, sem cair para o espelho.
# Em qualquer falha final o arquivo de destino é apagado e o script sai com erro.

foreach(_nl_var NL_URLS NL_DESTINO NL_SHA256)
    if("${${_nl_var}}" STREQUAL "")
        message(FATAL_ERROR "baixar_zip_xsd: ${_nl_var} vazio (passe com -D${_nl_var}=...)")
    endif()
endforeach()
string(LENGTH "${NL_SHA256}" _nl_sha_len)
if(NOT _nl_sha_len EQUAL 64 OR NOT NL_SHA256 MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "baixar_zip_xsd: NL_SHA256 precisa ter 64 hexadecimais minúsculos (achei ${_nl_sha_len} caracteres)")
endif()

# nl_baixar_url(URL): até 5 tentativas numa URL e, se baixou, a conferência do SHA-256. Devolve NL_RES_COD (0 = ok)
# e NL_RES_MSG. Divergência de hash devolve NL_RES_COD 2 com "HASH mismatch" na mensagem. Usa NL_DESTINO e NL_SHA256
# do escopo de quem chama.
function(nl_baixar_url _nl_url)
    set(_nl_tentativas 5)
    set(_nl_cod 1)
    set(_nl_msg "")
    foreach(_nl_tentativa RANGE 1 ${_nl_tentativas})
        file(DOWNLOAD "${_nl_url}" "${NL_DESTINO}"
            TLS_VERIFY ON
            TIMEOUT 120
            STATUS _nl_status)
        list(GET _nl_status 0 _nl_cod)
        if(_nl_cod EQUAL 0)
            break()
        endif()
        list(GET _nl_status 1 _nl_msg)
        if(_nl_tentativa EQUAL _nl_tentativas)
            break()
        endif()
        message(STATUS "baixar_zip_xsd: tentativa ${_nl_tentativa} de ${_nl_tentativas} falhou em ${_nl_url}: ${_nl_msg}")
        execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 10)
    endforeach()
    if(_nl_cod EQUAL 0)
        file(SHA256 "${NL_DESTINO}" _nl_obtido)
        if(NOT _nl_obtido STREQUAL NL_SHA256)
            set(_nl_cod 2)
            set(_nl_msg "HASH mismatch: SHA-256 esperado ${NL_SHA256}, obtido ${_nl_obtido}")
        endif()
    endif()
    set(NL_RES_COD "${_nl_cod}" PARENT_SCOPE)
    set(NL_RES_MSG "${_nl_msg}" PARENT_SCOPE)
endfunction()

# Laço pelas URLs: a primeira que servir o ZIP com o SHA-256 certo encerra; hash divergente encerra com erro.
# Arquivo já presente com o SHA-256 esperado não vai à rede (como o EXPECTED_HASH fazia antes).
set(_nl_servida "")
set(_nl_ultima_msg "")
set(_nl_urls_usar "${NL_URLS}")
if(EXISTS "${NL_DESTINO}")
    file(SHA256 "${NL_DESTINO}" _nl_ja_presente)
    if(_nl_ja_presente STREQUAL NL_SHA256)
        set(_nl_servida "arquivo já presente em ${NL_DESTINO}")
        set(_nl_urls_usar "")
    endif()
endif()
foreach(_nl_url IN LISTS _nl_urls_usar)
    nl_baixar_url("${_nl_url}")
    if(NL_RES_COD EQUAL 0)
        set(_nl_servida "${_nl_url}")
        break()
    endif()
    set(_nl_ultima_msg "${NL_RES_MSG}")
    if(NL_RES_MSG MATCHES "HASH mismatch")
        break()
    endif()
    message(STATUS "baixar_zip_xsd: ${_nl_url} falhou em 5 tentativas; passando à URL seguinte, se houver")
endforeach()

if(_nl_servida STREQUAL "")
    file(REMOVE "${NL_DESTINO}")
    message(FATAL_ERROR "baixar_zip_xsd: não consegui baixar o ZIP com SHA-256 esperado ${NL_SHA256} de nenhuma das URLs (${NL_URLS}): ${_nl_ultima_msg}")
endif()
message(STATUS "baixar_zip_xsd: ok, servido por ${_nl_servida} (${NL_DESTINO})")
