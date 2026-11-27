// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0238 (NL-3.L). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0238.h>

#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/tomador.h>

namespace nl::back {

std::string_view RegraE0238::codigo() const noexcept
{
    return "E0238";
}

std::optional<nl::base::Erro> RegraE0238::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    if (!dados.toma || !dados.toma->endereco()) {
        return std::nullopt;
    }
    const auto* nacional = std::get_if<EnderecoNacional>(&dados.toma->endereco()->local());
    if (nacional == nullptr || catalogo_.municipioExiste(nacional->cMun())) {
        return std::nullopt;
    }
    return nl::base::Erro{"E0238", std::string{"Dps.infDPS.toma.end.endNac.cMun"},
                          "o código do município do endereço do tomador não existe na tabela de municípios do IBGE (regra E0238, linha Excel 283)"};
}

} // namespace nl::back
