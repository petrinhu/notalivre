// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor da tributação municipal (NL-1.7), chave tribMun de valores: tribISSQN e tpRetISSQN obrigatórios, pAliq opcional.
#include "campos_json.h"
#include "leitores_comuns.h"
#include "leitores_valores.h"

#include <nl/back/dps/aliquota_issqn.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao_municipal.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::TributacaoMunicipal, nl::base::Erro> lerTributacaoMunicipal(const QJsonObject& valores, std::string_view caminhoValores)
{
    constexpr std::string_view chave = "tribMun";
    const auto tribMun = campos::objetoObrigatorio(valores, caminhoValores, chave);
    if (!tribMun) {
        return std::unexpected(tribMun.error());
    }
    const std::string caminho = campos::caminhoDe(caminhoValores, chave);

    const auto tribISSQNTexto = campos::textoObrigatorio(*tribMun, caminho, "tribISSQN");
    if (!tribISSQNTexto) {
        return std::unexpected(tribISSQNTexto.error());
    }
    const auto tribISSQN = nb::TribIssqn::de(*tribISSQNTexto);
    if (!tribISSQN) {
        return std::unexpected(tribISSQN.error());
    }

    const auto tpRetTexto = campos::textoObrigatorio(*tribMun, caminho, "tpRetISSQN");
    if (!tpRetTexto) {
        return std::unexpected(tpRetTexto.error());
    }
    const auto tpRetISSQN = nb::RetencaoIssqn::de(*tpRetTexto);
    if (!tpRetISSQN) {
        return std::unexpected(tpRetISSQN.error());
    }

    const auto pAliqTexto = campos::textoOpcional(*tribMun, caminho, "pAliq");
    if (!pAliqTexto) {
        return std::unexpected(pAliqTexto.error());
    }
    const auto pAliq = converterOpcional<nb::AliquotaIssqn>(*pAliqTexto, &nb::AliquotaIssqn::de);
    if (!pAliq) {
        return std::unexpected(pAliq.error());
    }

    return nb::TributacaoMunicipal::montar(*tribISSQN, *tpRetISSQN, *pAliq);
}

} // namespace nl::mid::composicao::leitores
