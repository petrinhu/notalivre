// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Plano do migrador (NL-3.1b, parte b1): esboço do vermelho; o verde põe a regra.

#include "nl/foundation/sql/migracao.h"

namespace nl::foundation::sql {

std::expected<std::vector<Migracao>, nl::base::Erro> planoDe(const std::vector<Migracao>& catalogo,
                                                            const std::vector<MigracaoAplicada>& aplicadas)
{
    (void)catalogo;
    (void)aplicadas;
    return std::unexpected(nl::base::Erro{"migracao.nao_implementado", std::nullopt, "plano ainda não implementado"});
}

} // namespace nl::foundation::sql
