// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de Dps (NL-1.4, NL-1.3 para o Id). O Id sai da montagem, nunca é recebido pronto.

#include <nl/back/dps/dps.h>

#include <utility>

namespace nl::back {

std::expected<Dps, nl::base::Erro> Dps::montar(DadosDps dados)
{
    if (!dados.dhEmi.isValid()) {
        return std::unexpected(nl::base::Erro{"dps.dhemi", std::string{"Dps.infDPS.dhEmi"}, "data e hora de emissão inválidas"});
    }
    if (!dados.dCompet.isValid()) {
        return std::unexpected(nl::base::Erro{"dps.dcompet", std::string{"Dps.infDPS.dCompet"}, "data de competência inválida"});
    }
    auto id = IdDps::montar(dados.cLocEmi, dados.prest.inscricao(), dados.serie, dados.nDps);
    if (!id) {
        return std::unexpected(id.error());
    }
    return Dps{std::move(*id), std::move(dados)};
}

} // namespace nl::back
