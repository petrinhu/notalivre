# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/pkcs12 (NL-2.2, ADR-S1): carregador de A1 sobre OpenSSL direto (sem QSslCertificate::importPkcs12).
# Motor agnóstico de domínio: não conhece Dps nem Tomador. Inscrição sai como texto (CNPJ pode ser alfanumérico).
# OpenSSL 3: o provider legacy é carregado só num OSSL_LIB_CTX próprio, nunca no processo inteiro.
find_package(OpenSSL 3.0 REQUIRED)

# Raízes da ICP-Brasil (NL-2.13, DA-2.47): os .der ficam em third_party, conferidos no configure, e nunca são compilados.
# O subdiretório exporta NL_ICP_RAIZES_DIR e NL_ICP_RAIZES_MANIFESTO, usados pelos testes da carga.
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/icp-brasil/raizes-20261030
                 ${CMAKE_BINARY_DIR}/third_party/icp-brasil/raizes-20261030)

add_library(nl_foundation_pkcs12 STATIC
    ancoras.cpp
    carga_raizes_icp.cpp
    classificacao_erros.cpp
    carregador_pkcs12.cpp
    conferencia_chave.cpp
    contexto_ossl.cpp
    credencial_x509.cpp
    decifrar_pfx.cpp
    extrator_inscricao.cpp
    inscricao.cpp
    manifesto_raizes.cpp
    senha_segura.cpp
    verificador_cadeia.cpp)
target_include_directories(nl_foundation_pkcs12 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nl_foundation_pkcs12 PUBLIC nl_foundation_base Qt6::Core)
target_link_libraries(nl_foundation_pkcs12 PRIVATE OpenSSL::Crypto nl_warnings)
