// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitores compartilhados por mais de um grupo da DPS (NL-1.7): inscrição de pessoa, endereço, regime e conversão opcional.
// Interno da camada Mid: não é API pública.
#ifndef NL_MID_COMPOSICAO_LEITORES_COMUNS_H
#define NL_MID_COMPOSICAO_LEITORES_COMUNS_H

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/base/erro.h>

#include <QJsonObject>
#include <QString>

#include <expected>
#include <optional>
#include <string_view>

namespace nl::mid::composicao::leitores {

/// Inscrição com CNPJ ou CPF da chave do objeto pai. Os dois juntos reprovam; nenhum é "entrada.campo_ausente".
std::expected<nl::back::Inscricao, nl::base::Erro> lerInscricao(const QJsonObject& pai, std::string_view caminhoPai);

/// Endereço nacional da chave "endereco" de pai. Ausente vira std::nullopt.
std::expected<std::optional<nl::back::Endereco>, nl::base::Erro> lerEnderecoOpcional(const QJsonObject& pai,
                                                                                     std::string_view caminhoPai);

/// Regime tributário da chave "regime" de pai (opSimpNac, regApTribSN opcional, regEspTrib).
std::expected<nl::back::RegimeTributacao, nl::base::Erro> lerRegime(const QJsonObject& pai, std::string_view caminhoPai);

/// Converte um texto opcional: ausente vira std::nullopt; presente passa pela fábrica do tipo do Back (que valida).
template <typename T, typename Fabrica>
std::expected<std::optional<T>, nl::base::Erro> converterOpcional(const std::optional<QString>& texto, Fabrica fabrica)
{
    if (!texto) {
        return std::optional<T>{std::nullopt};
    }
    auto valor = fabrica(*texto);
    if (!valor) {
        return std::unexpected(valor.error());
    }
    return std::optional<T>{*valor};
}

} // namespace nl::mid::composicao::leitores

#endif // NL_MID_COMPOSICAO_LEITORES_COMUNS_H
