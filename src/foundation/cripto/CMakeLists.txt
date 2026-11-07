# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Foundation/cripto (NL-3.2b, ADR-0003 e ADR-0006): AES-256-GCM, HMAC, HKDF e comparação sobre OpenSSL, e Argon2id
# sobre libsodium. Motor agnóstico: depende de base e de Qt Core (QByteArrayView). OpenSSL e libsodium ficam privados
# ao .cpp (nenhum tipo deles no cabeçalho). Não inclui pkcs12 (a NL-3.2c troca o interior da SenhaSegura, não aqui).
find_package(OpenSSL 3.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(NL_LIBSODIUM REQUIRED IMPORTED_TARGET libsodium)

add_library(nl_foundation_cripto STATIC
    aead.cpp
    aleatorio.cpp
    argon2id.cpp
    comparacao.cpp
    hkdf.cpp
    hmac.cpp
    senha_nfc.cpp)
target_include_directories(nl_foundation_cripto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(nl_foundation_cripto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/privado)
target_link_libraries(nl_foundation_cripto PUBLIC nl_foundation_base Qt6::Core)
target_link_libraries(nl_foundation_cripto PRIVATE OpenSSL::Crypto PkgConfig::NL_LIBSODIUM nl_warnings)
