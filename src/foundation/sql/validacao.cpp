// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco U-R (NL-3.1a): a validação entra no U-G. Devolve sql.nao_implementado para qualquer entrada.

#include "nl/foundation/sql/validacao.h"

#include <optional>
#include <utility>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> validarDadosConexao(const DadosConexao&)
{
    return std::unexpected(nl::base::Erro("sql.nao_implementado", std::nullopt, "não implementado"));
}

} // namespace nl::foundation::sql::detalhe
