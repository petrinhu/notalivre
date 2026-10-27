// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Estado de um número de DPS na numeração (NL-1.3). Enum fechado: um número está em exatamente um estado.

#ifndef NL_BACK_NUMERACAO_ESTADO_NUMERO_H
#define NL_BACK_NUMERACAO_ESTADO_NUMERO_H

#include <cstdint>

namespace nl::back {

/// Estado de uma reserva. Transições: Reservado -> Confirmado, Reservado -> Pulado. Nenhum estado volta a Reservado.
enum class EstadoNumero : std::uint8_t
{
    Reservado,
    Confirmado,
    Pulado,
};

} // namespace nl::back

#endif // NL_BACK_NUMERACAO_ESTADO_NUMERO_H
