// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Argon2id (NL-3.2b). Provisório do commit de testes: só devolve erro.

#include <nl/foundation/cripto/argon2id.h>

#include "nl/foundation/cripto/erros.h"

namespace nl::foundation::cripto {

std::expected<void, nl::base::Erro> argon2id(QByteArrayView, const ParametrosArgon2id&, std::span<unsigned char>)
{
    return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "primitiva ainda não implementada"));
}

} // namespace nl::foundation::cripto
