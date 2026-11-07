# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da Foundation de cripto (NL-3.2b, ADR-0003 e ADR-0006). Um executável por tema, registrado pela função de registro
# única (junitxml por função). O oráculo do OpenSSL só existe no build nativo: no Flatpak o binário openssl não está no SDK.
set(NL_CRIPTO_INCLUDE_PRIVADO ${CMAKE_SOURCE_DIR}/src/foundation/cripto/privado)

add_executable(tst_cripto_aead tst_cripto_aead.cpp)
target_link_libraries(tst_cripto_aead PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
nl_registra_teste_qt(NAME tst_cripto_aead COMMAND tst_cripto_aead)

add_executable(tst_cripto_primitivas tst_cripto_primitivas.cpp)
target_link_libraries(tst_cripto_primitivas PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
nl_registra_teste_qt(NAME tst_cripto_primitivas COMMAND tst_cripto_primitivas)

add_executable(tst_cripto_aleatorio tst_cripto_aleatorio.cpp)
target_link_libraries(tst_cripto_aleatorio PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
nl_registra_teste_qt(NAME tst_cripto_aleatorio COMMAND tst_cripto_aleatorio)

add_executable(tst_cripto_argon2id tst_cripto_argon2id.cpp)
target_link_libraries(tst_cripto_argon2id PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
nl_registra_teste_qt(NAME tst_cripto_argon2id COMMAND tst_cripto_argon2id)

add_executable(tst_cripto_senha_nfc tst_cripto_senha_nfc.cpp)
target_link_libraries(tst_cripto_senha_nfc PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
target_include_directories(tst_cripto_senha_nfc PRIVATE ${NL_CRIPTO_INCLUDE_PRIVADO})
nl_registra_teste_qt(NAME tst_cripto_senha_nfc COMMAND tst_cripto_senha_nfc)

if(NOT NL_FLATPAK_BUILD)
    add_executable(tst_cripto_argon2id_oraculo tst_cripto_argon2id_oraculo.cpp)
    target_link_libraries(tst_cripto_argon2id_oraculo PRIVATE Qt6::Core Qt6::Test nl_foundation_cripto nl_warnings)
    nl_registra_teste_qt(NAME tst_cripto_argon2id_oraculo COMMAND tst_cripto_argon2id_oraculo)
endif()
