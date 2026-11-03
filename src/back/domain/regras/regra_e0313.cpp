// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0313 (NL-2.10h, H1). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0313.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/dps/servico.h>

namespace nl::back {

std::string_view RegraE0313::codigo() const noexcept
{
    return "E0313";
}

std::optional<nl::base::Erro> RegraE0313::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    const bool mei = dados.prest.regime().opSimpNac().valor() == OpSimpNac::Valor::mei;
    if (!mei || !dados.serv.cTribMun()) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0313", std::string{"Dps.infDPS.serv.cServ.cTribMun"},
                          "Não é permitido informar cTribMun quando o emitente é MEI (regra E0313, linha Excel 340)"};
}

} // namespace nl::back
