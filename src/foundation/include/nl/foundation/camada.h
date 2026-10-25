// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Camada nl_foundation (NL-0.2): identifica a camada para o portão de dependências.

#ifndef NL_FOUNDATION_CAMADA_H
#define NL_FOUNDATION_CAMADA_H

#include <string_view>

namespace nl::foundation {

/// Nome da camada, usado pelos portões de camada.
inline constexpr std::string_view nomeCamada() noexcept
{
    return "foundation";
}

} // namespace nl::foundation

#endif // NL_FOUNDATION_CAMADA_H
