// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do total de tributos (NL-1.7; NL-2.10j, J1): chave totTrib de valores, uma das quatro formas do XSD (vTotTrib, pTotTrib, indTotTrib ou pTotTribSN). Só uma forma por DPS.
#include "campos_json.h"
#include "leitores_valores.h"

#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

namespace {

/// vTotTrib: os três valores monetários, todos obrigatórios quando a forma aparece.
std::expected<nb::TotalTributos, nl::base::Erro> lerMonetarios(const QJsonObject& totTrib, const std::string& caminho)
{
    const auto vFed = lerValorObrigatorio(totTrib, caminho, "vTotTribFed");
    if (!vFed) {
        return std::unexpected(vFed.error());
    }
    const auto vEst = lerValorObrigatorio(totTrib, caminho, "vTotTribEst");
    if (!vEst) {
        return std::unexpected(vEst.error());
    }
    const auto vMun = lerValorObrigatorio(totTrib, caminho, "vTotTribMun");
    if (!vMun) {
        return std::unexpected(vMun.error());
    }
    return nb::TotalTributos{nb::TributosMonetarios::montar(*vFed, *vEst, *vMun)};
}

/// Um percentual obrigatório (texto com ponto, como no leiaute). O tipo confere o padrão e a faixa do XSD.
std::expected<nb::PercentualTributo, nl::base::Erro> lerPercentual(const QJsonObject& totTrib, const std::string& caminho, const char* chave)
{
    const auto texto = campos::textoObrigatorio(totTrib, caminho, chave);
    if (!texto) {
        return std::unexpected(texto.error());
    }
    return nb::PercentualTributo::de(*texto);
}

/// pTotTrib: os três percentuais de tributo aproximado, na ordem do XSD.
std::expected<nb::TotalTributos, nl::base::Erro> lerPercentuais(const QJsonObject& totTrib, const std::string& caminho)
{
    const auto fed = lerPercentual(totTrib, caminho, "pTotTribFed");
    if (!fed) {
        return std::unexpected(fed.error());
    }
    const auto est = lerPercentual(totTrib, caminho, "pTotTribEst");
    if (!est) {
        return std::unexpected(est.error());
    }
    const auto mun = lerPercentual(totTrib, caminho, "pTotTribMun");
    if (!mun) {
        return std::unexpected(mun.error());
    }
    return nb::TotalTributos{nb::TributosPercentuais::montar(*fed, *est, *mun)};
}

/// indTotTrib: único valor 0 (o regime é conferido pela montagem da DPS, não aqui).
std::expected<nb::TotalTributos, nl::base::Erro> lerIndicador(const QString& texto)
{
    const auto indicador = nb::IndicadorTotalTributos::de(texto);
    if (!indicador) {
        return std::unexpected(indicador.error());
    }
    return nb::TotalTributos{*indicador};
}

/// pTotTribSN: percentual do Simples Nacional (TSDec2V2, 0 a 99,99).
std::expected<nb::TotalTributos, nl::base::Erro> lerSn(const QString& texto)
{
    const auto percentual = nb::PercentualTributoSn::de(texto);
    if (!percentual) {
        return std::unexpected(percentual.error());
    }
    return nb::TotalTributos{*percentual};
}

} // namespace

std::expected<nb::TotalTributos, nl::base::Erro> lerTotTrib(const QJsonObject& valores, std::string_view caminhoValores)
{
    constexpr std::string_view chave = "totTrib";
    const auto totTrib = campos::objetoObrigatorio(valores, caminhoValores, chave);
    if (!totTrib) {
        return std::unexpected(totTrib.error());
    }
    const std::string caminho = campos::caminhoDe(caminhoValores, chave);

    const auto indTexto = campos::textoOpcional(*totTrib, caminho, "indTotTrib");
    if (!indTexto) {
        return std::unexpected(indTexto.error());
    }
    const auto snTexto = campos::textoOpcional(*totTrib, caminho, "pTotTribSN");
    if (!snTexto) {
        return std::unexpected(snTexto.error());
    }
    bool temMonetario = false;
    bool temPercentual = false;
    for (const char* campo : {"vTotTribFed", "vTotTribEst", "vTotTribMun"}) {
        const auto texto = campos::textoOpcional(*totTrib, caminho, campo);
        if (!texto) {
            return std::unexpected(texto.error());
        }
        temMonetario = temMonetario || texto->has_value();
    }
    for (const char* campo : {"pTotTribFed", "pTotTribEst", "pTotTribMun"}) {
        const auto texto = campos::textoOpcional(*totTrib, caminho, campo);
        if (!texto) {
            return std::unexpected(texto.error());
        }
        temPercentual = temPercentual || texto->has_value();
    }
    const int formas = (indTexto->has_value() ? 1 : 0) + (snTexto->has_value() ? 1 : 0) + (temMonetario ? 1 : 0) + (temPercentual ? 1 : 0);
    if (formas > 1) {
        return std::unexpected(nl::base::Erro{"entrada.campos_conflitantes", caminho,
                                              "informe uma só forma de totTrib (indTotTrib, valores monetários, percentuais ou pTotTribSN), não os dois ou mais"});
    }
    if (indTexto->has_value()) {
        return lerIndicador(**indTexto);
    }
    if (snTexto->has_value()) {
        return lerSn(**snTexto);
    }
    if (temPercentual) {
        return lerPercentuais(*totTrib, caminho);
    }
    return lerMonetarios(*totTrib, caminho);
}

} // namespace nl::mid::composicao::leitores
