// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0241 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0241.h>
#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

namespace nl::back {

std::string_view RegraE0241::codigo() const noexcept
{
    return "E0241";
}

std::optional<nl::base::Erro> RegraE0241::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (dados.valores.trib().tribMun().tpRetISSQN().valor() != RetencaoIssqn::Valor::retidoPeloTomador) {
        return std::nullopt;
    }
    const bool identificadoPorCnpjOuCpf = dados.toma
        && (std::holds_alternative<Cnpj>(dados.toma->identificacao()) || std::holds_alternative<Cpf>(dados.toma->identificacao()));
    const bool comEnderecoNacional = dados.toma && dados.toma->endereco()
        && std::holds_alternative<EnderecoNacional>(dados.toma->endereco()->local());
    if (identificadoPorCnpjOuCpf && comEnderecoNacional) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0241", std::string{"Dps.infDPS.toma"},
                          "com retenção pelo tomador, o tomador é CNPJ ou CPF e tem endereço nacional (regra E0241, linha Excel 278, ramo da retenção)"};
}

} // namespace nl::back
