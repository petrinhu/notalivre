// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0237 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0237.h>
#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>

namespace nl::back {

std::string_view RegraE0237::codigo() const noexcept
{
    return "E0237";
}

std::optional<nl::base::Erro> RegraE0237::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (dados.valores.trib().tribMun().tpRetISSQN().valor() != RetencaoIssqn::Valor::retidoPeloTomador) {
        return std::nullopt;
    }
    if (dados.toma && dados.toma->endereco()
        && std::holds_alternative<EnderecoNacional>(dados.toma->endereco()->local())) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0237", std::string{"Dps.infDPS.toma.end.endNac"},
                          "retenção do ISSQN pelo tomador exige o endereço nacional do tomador (regra E0237, linha Excel 277)"};
}

} // namespace nl::back
