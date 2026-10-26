// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Reserva de um número de DPS (NL-1.3): par série e número. Valor puro, sem estado; o estado mora no repositório.

#ifndef NL_BACK_NUMERACAO_RESERVA_H
#define NL_BACK_NUMERACAO_RESERVA_H

#include <utility>

#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>

namespace nl::back {

/// Identidade de uma reserva: a série e o número. Dois valores iguais são a mesma reserva.
class Reserva
{
public:
    Reserva(Serie serie, NumeroDps numero)
        : serie_{std::move(serie)}, numero_{std::move(numero)}
    {
    }

    const Serie& serie() const noexcept { return serie_; }
    const NumeroDps& numero() const noexcept { return numero_; }

    bool operator==(const Reserva& outro) const = default;

private:
    Serie serie_;
    NumeroDps numero_;
};

} // namespace nl::back

#endif // NL_BACK_NUMERACAO_RESERVA_H
