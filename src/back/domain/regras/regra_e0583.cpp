// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0583 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0583.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>

namespace nl::back {

std::string_view RegraE0583::codigo() const noexcept
{
    return "E0583";
}

std::optional<nl::base::Erro> RegraE0583::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    const bool mei = dados.prest.regime().opSimpNac().valor() == OpSimpNac::Valor::mei;
    if (!mei || dados.valores.trib().tribMun().tpRetISSQN().valor() == RetencaoIssqn::Valor::naoRetido) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0583", std::string{"Dps.infDPS.valores.trib.tribMun.tpRetISSQN"},
                          "MEI não admite retenção do ISSQN: tpRetISSQN tem que ser 1 (regra E0583, linha Excel 513)"};
}

} // namespace nl::back
