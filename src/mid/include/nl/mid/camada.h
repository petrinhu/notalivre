// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Camada nl_mid (NL-0.2): identifica a camada para o portão de dependências.

#ifndef NL_MID_CAMADA_H
#define NL_MID_CAMADA_H

#include <string_view>

namespace nl::mid {

/// Nome da camada, usado pelos portões de camada.
inline constexpr std::string_view nomeCamada() noexcept
{
    return "mid";
}

} // namespace nl::mid

#endif // NL_MID_CAMADA_H
