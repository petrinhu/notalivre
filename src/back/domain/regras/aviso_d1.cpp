// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Aviso D1 (NL-1.5, ponto L10): tomador identificado por CNPJ sem endereço nacional. Só aviso, nunca erro.

#include <nl/back/regras/aviso_d1.h>

#include <variant>

#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/identificacao_pessoa.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/identificacao/cnpj.h>

namespace nl::back {

std::string_view AvisoD1::codigo() const noexcept
{
    return "D1";
}

std::optional<Aviso> AvisoD1::avaliar(const Dps& dps) const
{
    const auto& toma = dps.dados().toma;
    if (!toma || !std::holds_alternative<Cnpj>(toma->identificacao()) || toma->endereco()) {
        return std::nullopt;
    }
    return Aviso{"D1", "Dps.infDPS.toma.end",
                 "tomador identificado por CNPJ sem endereço nacional: aviso do site, não impede a emissão"};
}

} // namespace nl::back
