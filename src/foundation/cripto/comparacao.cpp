// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparação em tempo constante (NL-3.2b): CRYPTO_memcmp do OpenSSL, nunca == para HMAC ou valor de conferência.

#include <nl/foundation/cripto/comparacao.h>

#include <openssl/crypto.h>

namespace nl::foundation::cripto {

bool iguaisEmTempoConstante(QByteArrayView a, QByteArrayView b) noexcept
{
    if (a.size() != b.size()) {
        return false;
    }
    if (a.isEmpty()) {
        return true;
    }
    return CRYPTO_memcmp(a.data(), b.data(), static_cast<std::size_t>(a.size())) == 0;
}

} // namespace nl::foundation::cripto
