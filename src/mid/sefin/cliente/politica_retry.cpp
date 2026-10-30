// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de retry do ClienteSefin (NL-2.8): espera zero antes da primeira tentativa, depois dobra a cada repetição,
// com o expoente limitado (DA-2.41, d): 1 << (n - 2) com n grande é comportamento indefinido.
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <algorithm>
#include <cstdint>

namespace nl::mid::sefin::cliente {
namespace {

// Expoente máximo: 500 ms * 2^10 = 512 s, cerca de 8,5 minutos. Acima disso a espera não cresce mais.
constexpr int kExpoenteMaximo = 10;

} // namespace

std::chrono::milliseconds PoliticaRetry::esperaAntesDaTentativa(int n) const noexcept
{
    if (n < 2) {
        return std::chrono::milliseconds{0};
    }
    const int expoente = std::min(n - 2, kExpoenteMaximo);
    return base * (std::int64_t{1} << expoente);
}

} // namespace nl::mid::sefin::cliente
