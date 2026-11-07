// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// HMAC-SHA256 da Foundation de cripto (NL-3.2b, ADR-0003 e ADR-0004). Motor agnóstico.

#ifndef NL_FOUNDATION_CRIPTO_HMAC_H
#define NL_FOUNDATION_CRIPTO_HMAC_H

#include <nl/base/erro.h>

#include <QByteArrayView>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// HMAC-SHA256 (RFC 2104) com saída de 32 bytes na área do chamador. Falha da biblioteca: cripto.biblioteca.
[[nodiscard]] std::expected<void, nl::base::Erro> hmacSha256(QByteArrayView chave, QByteArrayView dado,
                                                              std::span<unsigned char, 32> saida);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_HMAC_H
