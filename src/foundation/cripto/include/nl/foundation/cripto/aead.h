// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// AES-256-GCM da Foundation de cripto (NL-3.2b, ADR-0003 regra 1). Motor agnóstico: não conhece campo nem domínio.
// O nonce vem de fora; quem sorteia é o chamador, por bytesAleatorios. A AAD é contexto autenticado, não é cifrada.

#ifndef NL_FOUNDATION_CRIPTO_AEAD_H
#define NL_FOUNDATION_CRIPTO_AEAD_H

#include <nl/base/erro.h>
#include <nl/foundation/cripto/tamanhos.h>

#include <QByteArray>
#include <QByteArrayView>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// Resultado da cifragem: o nonce usado e o texto cifrado com a tag de 16 bytes no fim.
struct Cifrado {
    QByteArray nonce;
    QByteArray textoComTag;
};

/// Cifra com AES-256-GCM. Chave de 32 bytes e nonce de 12 bytes, senão cripto.tamanho.
[[nodiscard]] std::expected<Cifrado, nl::base::Erro> cifrarAes256Gcm(QByteArrayView chave, QByteArrayView nonce,
                                                                      QByteArrayView aad, QByteArrayView claro);

/// Decifra com AES-256-GCM. A área de saída é do chamador. Texto menor que a tag: cripto.tamanho. Falha de tag:
/// cripto.tag. Em qualquer falha, a área de saída volta zerada, porque o claro pode ter sido escrito antes da conferência.
[[nodiscard]] std::expected<int, nl::base::Erro> decifrarAes256Gcm(QByteArrayView chave, QByteArrayView nonce,
                                                                    QByteArrayView aad, QByteArrayView textoComTag,
                                                                    std::span<unsigned char> claro);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_AEAD_H
