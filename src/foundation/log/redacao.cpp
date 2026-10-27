// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// STUB VERMELHO (NL-2.7, commit de teste): ignora a regra e devolve o valor cru.

#include <nl/foundation/log/redacao.h>

#include <optional>
#include <string>
#include <string_view>

namespace nl::foundation::log {

std::optional<std::string> aplicarRegra(Regra regra, std::string_view valor)
{
    static_cast<void>(regra);
    return std::string{valor};
}

} // namespace nl::foundation::log
