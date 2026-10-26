// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitores internos do grupo valores (NL-1.7): valor em pt-br, tribMun e totTrib. Interno da camada Mid: não é API pública.
#ifndef NL_MID_COMPOSICAO_LEITORES_VALORES_H
#define NL_MID_COMPOSICAO_LEITORES_VALORES_H

#include <nl/back/dinheiro/valor.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/base/erro.h>

#include <QJsonObject>

#include <expected>
#include <string_view>

namespace nl::mid::composicao::leitores {

/// Valor em pt-br ("100,00") de uma chave obrigatória de pai. O erro leva o caminho completo do campo.
std::expected<nl::back::Valor, nl::base::Erro> lerValorObrigatorio(const QJsonObject& pai, std::string_view caminhoPai,
                                                                   std::string_view chave);

/// tribMun: tribISSQN e tpRetISSQN obrigatórios, pAliq opcional.
std::expected<nl::back::TributacaoMunicipal, nl::base::Erro> lerTributacaoMunicipal(const QJsonObject& valores,
                                                                                     std::string_view caminhoValores);

/// totTrib: ou indTotTrib (o total não é informado por valor), ou os três valores monetários, nunca os dois.
std::expected<nl::back::TotalTributos, nl::base::Erro> lerTotTrib(const QJsonObject& valores, std::string_view caminhoValores);

} // namespace nl::mid::composicao::leitores

#endif // NL_MID_COMPOSICAO_LEITORES_VALORES_H
