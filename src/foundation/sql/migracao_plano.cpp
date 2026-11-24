// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Plano do migrador (NL-3.1b, parte b1): compara o que o banco já aplicou com o catálogo, sem escrever nada.

#include "nl/foundation/sql/migracao.h"

#include <algorithm>
#include <cstddef>

namespace nl::foundation::sql {

std::expected<std::vector<Migracao>, nl::base::Erro> planoDe(const std::vector<Migracao>& catalogo,
                                                            const std::vector<MigracaoAplicada>& aplicadas)
{
    std::vector<MigracaoAplicada> ordenadas = aplicadas;
    std::sort(ordenadas.begin(), ordenadas.end(),
              [](const MigracaoAplicada& a, const MigracaoAplicada& b) { return a.versao < b.versao; });
    const int maiorConhecida = catalogo.empty() ? 0 : catalogo.back().versao;
    if (!ordenadas.empty() && ordenadas.back().versao > maiorConhecida) {
        return std::unexpected(nl::base::Erro{"migracao.schema_mais_novo", std::nullopt,
                                              "o banco foi criado por uma versão mais nova do app; nada foi alterado"});
    }
    for (std::size_t i = 0; i < ordenadas.size(); ++i) {
        if (ordenadas[i].versao != static_cast<int>(i) + 1) {
            return std::unexpected(nl::base::Erro{"migracao.aplicadas_com_buraco", std::nullopt,
                                                  "a tabela de migrações do banco tem versão faltando ou repetida"});
        }
        if (ordenadas[i].checksum != catalogo[i].checksum) {
            return std::unexpected(nl::base::Erro{"migracao.checksum_divergente", std::nullopt,
                                                  "uma migração já aplicada não confere com o arquivo do app"});
        }
    }
    return std::vector<Migracao>(catalogo.begin() + static_cast<std::ptrdiff_t>(ordenadas.size()), catalogo.end());
}

} // namespace nl::foundation::sql
