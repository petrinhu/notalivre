// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0202 (NL-1.5). Ver o cabeçalho para a linha Excel e o texto. Função pura sobre a Dps já montada.

#include <nl/back/regras/regra_e0202.h>
#include <variant>

#include <nl/back/identificacao/cnpj.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/tomador.h>

namespace nl::back {

std::string_view RegraE0202::codigo() const noexcept
{
    return "E0202";
}

std::optional<nl::base::Erro> RegraE0202::verificar(const Dps& dps) const
{
    const auto& dados = dps.dados();
    const auto* cnpjPrestador = std::get_if<Cnpj>(&dados.prest.inscricao());
    if (cnpjPrestador == nullptr || !dados.toma) {
        return std::nullopt;
    }
    const auto* cnpjTomador = std::get_if<Cnpj>(&dados.toma->identificacao());
    if (cnpjTomador != nullptr && *cnpjTomador == *cnpjPrestador) {
        return nl::base::Erro{"E0202", std::string{"Dps.infDPS.toma.CNPJ"},
                              "CNPJ do tomador é igual ao CNPJ do prestador (regra E0202, linha Excel 257)"};
    }
    return std::nullopt;
}

} // namespace nl::back
