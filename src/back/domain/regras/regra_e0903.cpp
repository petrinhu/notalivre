// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0903 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0903.h>
#include <QString>

#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/tipo_operacao.h>

namespace nl::back {

std::string_view RegraE0903::codigo() const noexcept
{
    return "E0903";
}

std::optional<nl::base::Erro> RegraE0903::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    const QString codigo = dados.serv.cTribNac().texto();
    const bool listado = codigo.startsWith(QStringLiteral("2505")) || codigo.startsWith(QStringLiteral("1509"))
        || codigo.startsWith(QStringLiteral("1712")) || codigo.startsWith(QStringLiteral("1005"));
    if (!listado || (dados.ibscbs && dados.ibscbs->tpOper())) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0903", std::string{"Dps.infDPS.IBSCBS.tpOper"},
                          "tpOper obrigatório para cTribNac 25.05, 15.09, 17.12 e 10.05: preencha o grupo IBS/CBS com tpOper (regra E0903, linha Excel 574; ramo tpEnteGov fora do modelo)"};
}

} // namespace nl::back
