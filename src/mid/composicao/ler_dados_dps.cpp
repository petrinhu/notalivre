// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Composição dos grupos lidos do JSON em DadosDps (NL-1.7). Não valida regra: quem valida é cada tipo do Back e Dps::montar.
// Substituição (grupo subst) não entra na entrada da v1: a DPS sai sem subst (DA-1.3 e plano da Onda 1).
#include "leitores_dps.h"

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::DadosDps, nl::base::Erro> lerDadosDps(const QJsonObject& raiz)
{
    const auto identificacao = lerIdentificacao(raiz);
    if (!identificacao) {
        return std::unexpected(identificacao.error());
    }
    const auto prestador = lerPrestador(raiz);
    if (!prestador) {
        return std::unexpected(prestador.error());
    }
    const auto tomador = lerTomador(raiz);
    if (!tomador) {
        return std::unexpected(tomador.error());
    }
    const auto servico = lerServico(raiz);
    if (!servico) {
        return std::unexpected(servico.error());
    }
    const auto valores = lerValores(raiz);
    if (!valores) {
        return std::unexpected(valores.error());
    }
    const auto ibscbs = lerIbscbs(raiz);
    if (!ibscbs) {
        return std::unexpected(ibscbs.error());
    }

    return nb::DadosDps{
        .tpAmb = identificacao->tpAmb,
        .dhEmi = identificacao->dhEmi,
        .verAplic = identificacao->verAplic,
        .serie = identificacao->serie,
        .nDps = identificacao->nDps,
        .dCompet = identificacao->dCompet,
        .cLocEmi = identificacao->cLocEmi,
        .subst = std::nullopt,
        .prest = *prestador,
        .toma = *tomador,
        .serv = *servico,
        .valores = *valores,
        .ibscbs = *ibscbs,
    };
}

} // namespace nl::mid::composicao::leitores
