// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regras de redação do Log (NL-2.7, ADR-S4). Função pura, sem I/O. A regra é escolhida pela allowlist, nunca pelo chamador.

#ifndef NL_FOUNDATION_LOG_REDACAO_H
#define NL_FOUNDATION_LOG_REDACAO_H

#include <optional>
#include <string>
#include <string_view>

namespace nl::foundation::log {

/// Classe de redação de um campo declarado. Campo fora da allowlist nunca chega aqui (é descartado antes).
enum class Regra {
    Cru,                 ///< aparece como está
    InscricaoMascarada,  ///< Id da DPS: as 14 posições da inscrição (12 a 25, 1-based) viram '#'
    Proibido,            ///< nunca aparece
};

/// Aplica a regra ao valor. nullopt quer dizer que o valor não pode ser gravado: regra Proibido, ou valor fora do
/// formato esperado (fail-closed). Quem chama descarta o campo e conta o descarte, sem gravar o valor.
std::optional<std::string> aplicarRegra(Regra regra, std::string_view valor);

/// Nome de campo da classe Proibido da ADR-0005 §1 (senha, token, CPF de tomador, XML, corpo de e-mail etc.).
/// Vale sobre a allowlist: quem declara o campo como Cru ou InscricaoMascarada não muda o descarte.
bool ehCampoProibido(std::string_view nome) noexcept;

} // namespace nl::foundation::log

#endif // NL_FOUNDATION_LOG_REDACAO_H
