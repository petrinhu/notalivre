// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta do GET_LOCK do migrador (NL-3.1b, parte b2; parecer do CISO, I-a). Um nulo do servidor nunca vira trava dada.

#include "nl/foundation/sql/trava_migracao.h"

#include <optional>
#include <variant>

namespace nl::foundation::sql::detalhe {
namespace {

// O inteiro da resposta, só quando ela é uma linha com uma coluna inteira.
std::optional<qint64> inteiroUnico(const Linhas& linhas)
{
    if (linhas.size() != 1 || linhas.front().size() != 1) {
        return std::nullopt;
    }
    if (const auto* inteiro = std::get_if<qint64>(&linhas.front().front())) {
        return *inteiro;
    }
    return std::nullopt;
}

} // namespace

std::expected<void, nl::base::Erro> respostaDaTrava(const Linhas& linhas)
{
    const std::optional<qint64> resposta = inteiroUnico(linhas);
    if (resposta == qint64{0}) {
        return std::unexpected(nl::base::Erro{"migracao.trava_ocupada", std::nullopt,
                                              "outra sessão está migrando este banco; tente de novo em instantes"});
    }
    if (resposta != qint64{1}) {
        return std::unexpected(nl::base::Erro{"migracao.trava_falhou", std::nullopt, "o servidor não deu a trava da migração"});
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
