// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Esboço da resposta do GET_LOCK (NL-3.1b, parte b2, passo R): toda chamada devolve sql.nao_implementado.

#include "nl/foundation/sql/trava_migracao.h"

#include <optional>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> respostaDaTrava(const Linhas&)
{
    return std::unexpected(nl::base::Erro{"sql.nao_implementado", std::nullopt, "resposta da trava ainda não implementada"});
}

} // namespace nl::foundation::sql::detalhe
