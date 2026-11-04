// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tamanhos fixos das primitivas de cripto (NL-3.2b, ADR-0003 e ADR-0006). Constantes; nenhum tipo de terceiro.

#ifndef NL_FOUNDATION_CRIPTO_TAMANHOS_H
#define NL_FOUNDATION_CRIPTO_TAMANHOS_H

namespace nl::foundation::cripto {

/// Chave AES-256 e HMAC-SHA256 (32 bytes).
inline constexpr int kTamanhoChave = 32;
/// Nonce do AES-GCM de 96 bits (NIST SP 800-38D, 5.2.1.1). Quem sorteia é o chamador, por bytesAleatorios.
inline constexpr int kTamanhoNonce = 12;
/// Tag do AES-GCM: 128 bits, fixa. Tag truncada não é aceita (NIST SP 800-38D, 5.2.1.2).
inline constexpr int kTamanhoTag = 16;
/// Salt do Argon2id (ADR-0006, item 2).
inline constexpr int kTamanhoSalt = 16;

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_TAMANHOS_H
