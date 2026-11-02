// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de Dps (NL-1.4, NL-1.3 para o Id). O Id sai da montagem, nunca é recebido pronto.

#include <nl/back/dps/dps.h>

#include <nl/back/dps/total_tributos_faixa.h>
#include <nl/back/dps/total_tributos_regime.h>

#include <utility>

namespace nl::back {

std::expected<Dps, nl::base::Erro> Dps::montar(DadosDps dados)
{
    auto dhEmi = DhEmi::de(dados.dhEmi);
    if (!dhEmi) {
        return std::unexpected(dhEmi.error());
    }
    if (!dados.dCompet.isValid()) {
        return std::unexpected(nl::base::Erro{"dps.dcompet", std::string{"Dps.infDPS.dCompet"}, "data de competência inválida"});
    }
    // NL-2.10j (J1): a forma de totTrib tem de caber no regime e nas faixas antes de qualquer envio ou reserva de número.
    if (auto erro = verificarTotTribRegime(dados)) {
        return std::unexpected(*erro);
    }
    if (auto erro = verificarTotTribFaixa(dados)) {
        return std::unexpected(*erro);
    }
    auto id = IdDps::montar(dados.cLocEmi, dados.prest.inscricao(), dados.serie, dados.nDps);
    if (!id) {
        return std::unexpected(id.error());
    }
    return Dps{std::move(*id), std::move(*dhEmi), std::move(dados)};
}

} // namespace nl::back
