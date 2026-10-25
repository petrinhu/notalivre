// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Camada nl_foundation_base (NL-0.2): identifica a camada para o portão de dependências e para o teste de sanidade.

#ifndef NL_BASE_CAMADA_H
#define NL_BASE_CAMADA_H

#include <string_view>

namespace nl::base {

/// Nome da camada, usado pelos testes de sanidade e pelos portões de camada.
inline constexpr std::string_view nomeCamada() noexcept
{
    return "base";
}

} // namespace nl::base

#endif // NL_BASE_CAMADA_H
