// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de retry do ClienteSefin (NL-2.8): espera zero antes da primeira tentativa, depois dobra a cada repetição.
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <cstdint>

namespace nl::mid::sefin::cliente {

std::chrono::milliseconds PoliticaRetry::esperaAntesDaTentativa(int n) const noexcept
{
    if (n < 2) {
        return std::chrono::milliseconds{0};
    }
    return base * (std::int64_t{1} << (n - 2));
}

} // namespace nl::mid::sefin::cliente
