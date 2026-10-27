// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0302 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0302.h>
#include <QString>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/servico.h>

namespace nl::back {

std::string_view RegraE0302::codigo() const noexcept
{
    return "E0302";
}

std::optional<nl::base::Erro> RegraE0302::verificar(const Dps& dps) const
{
    // O código 0000000 (Águas Marítimas) é uma linha da TAB.LOC.GERAL e é consultado no port como os demais (DA-1.10 (a)).
    const auto& cLocPrestacao = dps.dados().serv.cLocPrestacao();
    if (catalogo_.municipioExiste(cLocPrestacao)) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0302", std::string{"Dps.infDPS.serv.locPrest.cLocPrestacao"},
                          "cLocPrestacao não existe na tabela de municípios do IBGE (regra E0302, linha Excel 333)"};
}

} // namespace nl::back
