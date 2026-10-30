// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.8): espera zero em toda tentativa. A política real entra no commit verde.
#include <nl/mid/sefin/cliente/politica_retry.h>

namespace nl::mid::sefin::cliente {

std::chrono::milliseconds PoliticaRetry::esperaAntesDaTentativa(int) const noexcept
{
    return std::chrono::milliseconds{0};
}

} // namespace nl::mid::sefin::cliente
