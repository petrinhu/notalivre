// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco U-R (NL-3.1a): a montagem dos vínculos entra no U-G. Devolve sql.nao_implementado.

#include "nl/foundation/sql/vinculos.h"

#include <optional>
#include <utility>

namespace nl::foundation::sql::detalhe {

std::expected<VinculosMontados, nl::base::Erro> montarVinculos(std::span<const Parametro>)
{
    return std::unexpected(nl::base::Erro("sql.nao_implementado", std::nullopt, "não implementado"));
}

} // namespace nl::foundation::sql::detalhe
