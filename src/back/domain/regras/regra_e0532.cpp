// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0532 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0532.h>
#include <QString>

#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>

namespace nl::back {

std::string_view RegraE0532::codigo() const noexcept
{
    return "E0532";
}

std::optional<nl::base::Erro> RegraE0532::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (!dados.serv.cTribNac().texto().startsWith(QStringLiteral("99"))) {
        return std::nullopt;
    }
    if (dados.valores.trib().tribMun().tribISSQN().valor() == TribIssqn::Valor::naoIncidencia) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0532", std::string{"Dps.infDPS.valores.trib.tribMun.tribISSQN"},
                          "cTribNac 99.XX.XX exige tribISSQN 4, não incidência (regra E0532, linha Excel 484)"};
}

} // namespace nl::back
