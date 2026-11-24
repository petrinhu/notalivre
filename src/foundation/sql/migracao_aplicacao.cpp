// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Esboço do migrador no banco (NL-3.1b, parte b2, passo R): toda chamada devolve sql.nao_implementado.

#include "nl/foundation/sql/migrador.h"

#include <optional>

namespace nl::foundation::sql {

std::expected<ResultadoMigracao, nl::base::Erro> migrar(Executor&, std::span<const MigracaoEmbutida>)
{
    return std::unexpected(nl::base::Erro{"sql.nao_implementado", std::nullopt, "migrador ainda não implementado"});
}

} // namespace nl::foundation::sql
