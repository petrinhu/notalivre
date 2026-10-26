// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitores de cada grupo de raiz do JSON de entrada (NL-1.7), um arquivo por grupo. Interno da camada Mid: não é API pública.
// Cada leitor devolve o tipo do Back já validado, ou o primeiro erro (campo do JSON ou regra do Back).
// Os leitores compartilhados entre grupos moram em leitores_comuns.h; os do grupo valores, em leitores_valores.h.
#ifndef NL_MID_COMPOSICAO_LEITORES_DPS_H
#define NL_MID_COMPOSICAO_LEITORES_DPS_H

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/valores_servico.h>
#include <nl/base/erro.h>

#include <QDate>
#include <QDateTime>
#include <QJsonObject>

#include <expected>
#include <optional>

namespace nl::mid::composicao::leitores {

/// Campos de identificação da infDPS: tpAmb, dhEmi, verAplic, serie, nDPS, dCompet e cLocEmi.
struct Identificacao
{
    nl::back::TipoAmbiente tpAmb;
    QDateTime dhEmi;
    nl::back::VerAplic verAplic;
    nl::back::Serie serie;
    nl::back::NumeroDps nDps;
    QDate dCompet;
    nl::back::CodigoMunicipio cLocEmi;
};

/// Grupo de raiz: lê o JSON inteiro e devolve os dados que Dps::montar recebe.
std::expected<nl::back::DadosDps, nl::base::Erro> lerDadosDps(const QJsonObject& raiz);

std::expected<Identificacao, nl::base::Erro> lerIdentificacao(const QJsonObject& raiz);

std::expected<nl::back::Prestador, nl::base::Erro> lerPrestador(const QJsonObject& raiz);

/// Tomador é opcional na DPS: ausente vira std::nullopt.
std::expected<std::optional<nl::back::Tomador>, nl::base::Erro> lerTomador(const QJsonObject& raiz);

std::expected<nl::back::Servico, nl::base::Erro> lerServico(const QJsonObject& raiz);

/// Valores do serviço: vServ, tribMun e totTrib.
std::expected<nl::back::ValoresServico, nl::base::Erro> lerValores(const QJsonObject& raiz);

/// Grupo IBS/CBS é opcional na DPS: ausente vira std::nullopt.
std::expected<std::optional<nl::back::Ibscbs>, nl::base::Erro> lerIbscbs(const QJsonObject& raiz);

} // namespace nl::mid::composicao::leitores

#endif // NL_MID_COMPOSICAO_LEITORES_DPS_H
