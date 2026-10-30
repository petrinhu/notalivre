// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.8): nenhum código lido. O leitor real entra no commit verde.
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>

namespace nl::mid::sefin::cliente {

std::optional<std::string> codigoDoErro(const QByteArray&)
{
    return std::nullopt;
}

} // namespace nl::mid::sefin::cliente
