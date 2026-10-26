// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0850 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0850.h>

#include <QDate>
#include <nl/back/dps/dados_dps.h>

namespace nl::back {

std::string_view RegraE0850::codigo() const noexcept
{
    return "E0850";
}

std::optional<nl::base::Erro> RegraE0850::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (!dados.ibscbs || dados.dCompet >= QDate{2026, 1, 1}) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0850", std::string{"Dps.infDPS.IBSCBS"},
                          "IBS/CBS só com competência a partir de 01/01/2026 (regra E0850, linha Excel 566)"};
}

} // namespace nl::back
