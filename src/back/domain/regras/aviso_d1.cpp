// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Aviso D1 (NL-1.5, ponto L10): tomador identificado por CNPJ sem endereço nacional. Só aviso, nunca erro.
// Fonte: a E0235 (linha Excel 276 da aba RN do Anexo I de 20/10/2026) tem K e N = X, então a Sefin não a aplica à DPS
// do app; a regra só do site vira aviso pela decisão do líder no ponto L10 (plano, NL-1.5; DA-1.19).

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
