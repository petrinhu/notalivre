// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação da regra de regime do totTrib (NL-2.10j, J1). Ver o cabeçalho para as linhas do Anexo I.

#include <nl/back/dps/total_tributos_regime.h>

#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/tributacao.h>

#include <string>
#include <variant>

namespace nl::back {

std::optional<nl::base::Erro> verificarTotTribRegime(const DadosDps& dados)
{
    const OpSimpNac::Valor regime = dados.prest.regime().opSimpNac().valor();
    const TotalTributos& total = dados.valores.trib().totTrib();
    const bool indicador = std::holds_alternative<IndicadorTotalTributos>(total);
    const bool percentualSn = std::holds_alternative<PercentualTributoSn>(total);
    if (indicador && regime == OpSimpNac::Valor::meEpp) {
        return nl::base::Erro{"dps.tottrib_regime", std::string{"Dps.infDPS.valores.trib.totTrib.indTotTrib"},
                              "ME/EPP não informa indTotTrib (regra E0712, linha Excel 562)"};
    }
    if (indicador && regime == OpSimpNac::Valor::naoOptante) {
        return nl::base::Erro{"dps.tottrib_regime", std::string{"Dps.infDPS.valores.trib.totTrib.indTotTrib"},
                              "não optante do Simples não informa indTotTrib (regra E0713, linha Excel 563)"};
    }
    if (percentualSn && regime == OpSimpNac::Valor::naoOptante) {
        return nl::base::Erro{"dps.tottrib_regime", std::string{"Dps.infDPS.valores.trib.totTrib.pTotTribSN"},
                              "não optante do Simples não informa pTotTribSN (regra E0713, linha Excel 563)"};
    }
    if (percentualSn && regime == OpSimpNac::Valor::mei) {
        return nl::base::Erro{"dps.tottrib_regime", std::string{"Dps.infDPS.valores.trib.totTrib.pTotTribSN"},
                              "MEI não informa pTotTribSN (regra E0710, linha Excel 565)"};
    }
    return std::nullopt;
}

} // namespace nl::back
