// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// AES-256-GCM (NL-3.2b). Provisório do commit de testes: só devolve erro, para o vermelho aparecer por asserção.

#include <nl/foundation/cripto/aead.h>

#include "nl/foundation/cripto/erros.h"

namespace nl::foundation::cripto {

std::expected<Cifrado, nl::base::Erro> cifrarAes256Gcm(QByteArrayView, QByteArrayView, QByteArrayView, QByteArrayView)
{
    return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "primitiva ainda não implementada"));
}

std::expected<int, nl::base::Erro> decifrarAes256Gcm(QByteArrayView, QByteArrayView, QByteArrayView, QByteArrayView,
                                                      std::span<unsigned char>)
{
    return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "primitiva ainda não implementada"));
}

} // namespace nl::foundation::cripto
