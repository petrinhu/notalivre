// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação das faixas do totTrib (NL-2.10j, J1). Ver o cabeçalho para as linhas do Anexo I.

#include <nl/back/dps/total_tributos_faixa.h>

#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>

#include <QString>

#include <string>
#include <variant>

namespace nl::back {

namespace {

/// Percentual já validado pelo tipo (TSDec3V2) está em 0 a 100%? Compara a parte inteira e as casas, sem ponto flutuante.
bool ateCemPorCento(const PercentualTributo& percentual)
{
    const QString texto = percentual.texto();
    const qsizetype ponto = texto.indexOf(QLatin1Char('.'));
    const int inteiro = (ponto < 0 ? texto : texto.left(ponto)).toInt();
    if (inteiro < 100) {
        return true;
    }
    if (inteiro > 100) {
        return false;
    }
    const QString casas = ponto < 0 ? QString{} : texto.mid(ponto + 1);
    return casas.count(QLatin1Char('0')) == casas.size();
}

std::optional<nl::base::Erro> erroDeFaixa(const std::string& campo, const std::string& mensagem)
{
    return nl::base::Erro{"dps.tottrib_faixa", campo, mensagem};
}

} // namespace

std::optional<nl::base::Erro> verificarTotTribFaixa(const DadosDps& dados)
{
    const TotalTributos& total = dados.valores.trib().totTrib();
    const std::string grupo = "Dps.infDPS.valores.trib.totTrib";
    if (const auto* monetarios = std::get_if<TributosMonetarios>(&total)) {
        const auto teto = dados.valores.vServ().centavos();
        if (monetarios->vTotTribFed().centavos() > teto) {
            return erroDeFaixa(grupo + ".vTotTrib.vTotTribFed", "vTotTribFed acima do valor do serviço (regra E0702, linha Excel 555)");
        }
        if (monetarios->vTotTribEst().centavos() > teto) {
            return erroDeFaixa(grupo + ".vTotTrib.vTotTribEst", "vTotTribEst acima do valor do serviço (regra E0703, linha Excel 556)");
        }
        if (monetarios->vTotTribMun().centavos() > teto) {
            return erroDeFaixa(grupo + ".vTotTrib.vTotTribMun", "vTotTribMun acima do valor do serviço (regra E0704, linha Excel 557)");
        }
    }
    if (const auto* percentuais = std::get_if<TributosPercentuais>(&total)) {
        if (!ateCemPorCento(percentuais->pTotTribFed())) {
            return erroDeFaixa(grupo + ".pTotTrib.pTotTribFed", "pTotTribFed acima de 100% (regra E0706, linha Excel 559)");
        }
        if (!ateCemPorCento(percentuais->pTotTribEst())) {
            return erroDeFaixa(grupo + ".pTotTrib.pTotTribEst", "pTotTribEst acima de 100% (regra E0707, linha Excel 560)");
        }
        if (!ateCemPorCento(percentuais->pTotTribMun())) {
            return erroDeFaixa(grupo + ".pTotTrib.pTotTribMun", "pTotTribMun acima de 100% (regra E0708, linha Excel 561)");
        }
    }
    return std::nullopt;
}

} // namespace nl::back
