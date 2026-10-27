// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do tipo Erro da camada nl_foundation_base (NL-1.12).

#include <nl/base/erro.h>

#include <optional>
#include <string>
#include <utility>

namespace nl::base {

Erro::Erro(std::string codigo, std::optional<std::string> campo, std::string mensagem)
    : codigo_{std::move(codigo)}, campo_{std::move(campo)}, mensagem_{std::move(mensagem)}
{
}

bool operator==(const Erro& esquerdo, const Erro& direito) noexcept
{
    return esquerdo.codigo() == direito.codigo() && esquerdo.campo() == direito.campo();
}

} // namespace nl::base
