// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Forma canônica da senha (NL-3.2b). Provisório do commit de testes: só devolve erro.

#include <nl/foundation/cripto/senha_nfc.h>

#include "nl/foundation/cripto/erros.h"
#include "nl/foundation/cripto/senha_nfc_interno.h"

namespace nl::foundation::cripto {

std::expected<int, nl::base::Erro> senhaEmNfc(QByteArrayView utf8, std::span<unsigned char> destino)
{
    detalhe::RelatorioNfc relatorio;
    return detalhe::senhaEmNfcComRelatorio(utf8, destino, relatorio);
}

namespace detalhe {

std::expected<int, nl::base::Erro> senhaEmNfcComRelatorio(QByteArrayView, std::span<unsigned char>, RelatorioNfc&)
{
    return std::unexpected(erroCripto("cripto.biblioteca", "primitiva ainda não implementada"));
}

} // namespace detalhe

} // namespace nl::foundation::cripto
