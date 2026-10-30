// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de retry do ClienteSefin (NL-2.8). Decisão do CTO a confirmar: o número de tentativas é configurável e só
// é medido na NL-2.10 e na Onda 11. Valor sem I/O: o tempo de espera é calculado, nunca dormido aqui.
#ifndef NL_MID_SEFIN_CLIENTE_POLITICA_RETRY_H
#define NL_MID_SEFIN_CLIENTE_POLITICA_RETRY_H

#include <chrono>

namespace nl::mid::sefin::cliente {

struct PoliticaRetry {
    int tentativas = 3;                       ///< Total de tentativas de GET e HEAD em erro transitório (1 = sem repetir).
    std::chrono::milliseconds base{500};      ///< Primeira espera; a seguinte dobra (backoff exponencial).

    /// Espera antes da tentativa n (n = 2 é a primeira repetição): base * 2^(n-2). Zero para n < 2.
    [[nodiscard]] std::chrono::milliseconds esperaAntesDaTentativa(int n) const noexcept;
};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_POLITICA_RETRY_H
