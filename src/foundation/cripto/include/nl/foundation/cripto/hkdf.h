// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// HKDF-SHA256 da Foundation de cripto (NL-3.2b, RFC 5869). Motor agnóstico. Salt e info vazios são válidos.

#ifndef NL_FOUNDATION_CRIPTO_HKDF_H
#define NL_FOUNDATION_CRIPTO_HKDF_H

#include <nl/base/erro.h>

#include <QByteArrayView>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// Deriva a área de saída a partir do material de chave (RFC 5869, extract e expand com SHA-256).
[[nodiscard]] std::expected<void, nl::base::Erro> hkdfSha256(QByteArrayView chave, QByteArrayView salt,
                                                              QByteArrayView info, std::span<unsigned char> saida);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_HKDF_H
