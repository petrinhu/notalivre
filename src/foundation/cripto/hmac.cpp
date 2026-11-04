// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// HMAC-SHA256 (NL-3.2b). Provisório do commit de testes: só devolve erro.

#include <nl/foundation/cripto/hmac.h>

#include "nl/foundation/cripto/erros.h"

namespace nl::foundation::cripto {

std::expected<void, nl::base::Erro> hmacSha256(QByteArrayView, QByteArrayView, std::span<unsigned char, 32>)
{
    return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "primitiva ainda não implementada"));
}

} // namespace nl::foundation::cripto
