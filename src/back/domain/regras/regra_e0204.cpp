// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0204 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0204.h>
#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

namespace nl::back {

std::string_view RegraE0204::codigo() const noexcept
{
    return "E0204";
}

std::optional<nl::base::Erro> RegraE0204::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (dados.valores.trib().tribMun().tpRetISSQN().valor() != RetencaoIssqn::Valor::retidoPeloTomador) {
        return std::nullopt;
    }
    if (dados.toma) {
        const auto& identificacao = dados.toma->identificacao();
        if (std::holds_alternative<Cnpj>(identificacao) || std::holds_alternative<Cpf>(identificacao)) {
            return std::nullopt;
        }
    }
    return nl::base::Erro{"E0204", std::string{"Dps.infDPS.toma"},
                          "retenção do ISSQN pelo tomador exige o tomador identificado por CNPJ ou CPF (regra E0204, linha Excel 258)"};
}

} // namespace nl::back
