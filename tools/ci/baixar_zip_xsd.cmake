# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Download do ZIP do XSD de Produção Restrita no CI (NL-1.6c). Roda com `cmake -P`, presente nas quatro imagens
# de build. O cliente é o do CMake (file(DOWNLOAD)): o gov.br recusou o urllib do Python com 403 (run 40268530187).
# Uso: cmake -DNL_URL=<url> -DNL_DESTINO=<arquivo> -DNL_SHA256=<64 hex minúsculos> -P tools/ci/baixar_zip_xsd.cmake
# Até 5 tentativas para falha de transporte. Divergência de hash é fatal no próprio file(DOWNLOAD) do CMake
# (mesmo com STATUS): para na primeira, sem nova tentativa, e o arquivo fica no disco. Quem chama apaga
# (o passo do ci.yml faz rm -f "$zip" quando o cmake -P sai com erro). EXPECTED_HASH confere o SHA-256 no download.

foreach(_nl_var NL_URL NL_DESTINO NL_SHA256)
    if("${${_nl_var}}" STREQUAL "")
        message(FATAL_ERROR "baixar_zip_xsd: ${_nl_var} vazio (passe com -D${_nl_var}=...)")
    endif()
endforeach()
string(LENGTH "${NL_SHA256}" _nl_sha_len)
if(NOT _nl_sha_len EQUAL 64 OR NOT NL_SHA256 MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "baixar_zip_xsd: NL_SHA256 precisa ter 64 hexadecimais minúsculos (achei ${_nl_sha_len} caracteres)")
endif()

set(_nl_tentativas 5)
set(_nl_cod 1)
set(_nl_msg "")
foreach(_nl_tentativa RANGE 1 ${_nl_tentativas})
    file(DOWNLOAD "${NL_URL}" "${NL_DESTINO}"
        EXPECTED_HASH SHA256=${NL_SHA256}
        TLS_VERIFY ON
        TIMEOUT 120
        STATUS _nl_status)
    list(GET _nl_status 0 _nl_cod)
    if(_nl_cod EQUAL 0)
        message(STATUS "baixar_zip_xsd: ok na tentativa ${_nl_tentativa} de ${_nl_tentativas} (${NL_DESTINO})")
        break()
    endif()
    list(GET _nl_status 1 _nl_msg)
    if(_nl_msg MATCHES "HASH mismatch" OR _nl_tentativa EQUAL _nl_tentativas)
        break()
    endif()
    message(STATUS "baixar_zip_xsd: tentativa ${_nl_tentativa} de ${_nl_tentativas} falhou: ${_nl_msg}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 10)
endforeach()

if(NOT _nl_cod EQUAL 0)
    file(REMOVE "${NL_DESTINO}")
    message(FATAL_ERROR "baixar_zip_xsd: não consegui baixar ${NL_URL} com SHA-256 esperado ${NL_SHA256}: ${_nl_msg}")
endif()
