// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regras de redação do Log (NL-2.7, ADR-S4). Função pura, sem I/O. A regra é escolhida pela allowlist, nunca pelo chamador.

#ifndef NL_FOUNDATION_LOG_REDACAO_H
#define NL_FOUNDATION_LOG_REDACAO_H

#include <optional>
#include <span>
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

/// Nome de campo da classe Proibido da ADR-0005 §1 e da DA-2.23. Compara sem caixa e por radical sensível (lado
/// fechado): nome vazio, não ASCII, igual a um da lista ou que contém um radical é proibido. Vale sobre a allowlist.
bool ehCampoProibido(std::string_view nome);

/// idDps, idNfse e chaveAcesso (DA-2.23): só aceitam a regra InscricaoMascarada. Declarados de outro jeito são descartados.
bool ehCampoSoMascara(std::string_view nome);

/// Radicais do lado fechado de ehCampoProibido (comparação sem caixa). Todo nome de camposProibidosDeReferencia()
/// tem de conter ao menos um destes: o teste radicaisCobremTodosOsNomesProibidos confere.
std::span<const std::string_view> radicaisDeCampoProibido() noexcept;

/// Nomes exatos da classe Proibido (ADR-0005 §1 e DA-2.23), em forma de referência para testes e auditoria.
std::span<const std::string_view> camposProibidosDeReferencia() noexcept;

} // namespace nl::foundation::log

#endif // NL_FOUNDATION_LOG_REDACAO_H
