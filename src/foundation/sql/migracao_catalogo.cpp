// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo do migrador (NL-3.1b, parte b1): esboço do vermelho; o verde põe a regra.

#include "nl/foundation/sql/migracao.h"

namespace nl::foundation::sql {

std::expected<std::vector<Migracao>, nl::base::Erro> catalogoDe(const std::vector<ArquivoMigracao>& arquivos)
{
    (void)arquivos;
    return std::unexpected(nl::base::Erro{"migracao.nao_implementado", std::nullopt, "catálogo ainda não implementado"});
}

} // namespace nl::foundation::sql
