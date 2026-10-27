// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0710 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0710.h>
#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributacao.h>

namespace nl::back {

std::string_view RegraE0710::codigo() const noexcept
{
    return "E0710";
}

std::optional<nl::base::Erro> RegraE0710::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    const bool mei = dados.prest.regime().opSimpNac().valor() == OpSimpNac::Valor::mei;
    if (!mei || !std::holds_alternative<PercentualTributoSn>(dados.valores.trib().totTrib())) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0710", std::string{"Dps.infDPS.valores.trib.totTrib.pTotTribSN"},
                          "MEI nunca informa pTotTribSN (regra E0710, linha Excel 565)"};
}

} // namespace nl::back
