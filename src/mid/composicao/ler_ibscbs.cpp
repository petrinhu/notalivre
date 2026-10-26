// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do grupo IBS/CBS (NL-1.7), opcional: cIndOp e tpOper opcionais; indDest, cst e cClassTrib obrigatórios.
#include "campos_json.h"
#include "leitores_comuns.h"
#include "leitores_dps.h"

#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_indicador_operacao.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/tipo_operacao.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<std::optional<nb::Ibscbs>, nl::base::Erro> lerIbscbs(const QJsonObject& raiz)
{
    constexpr std::string_view caminho = "ibscbs";
    const auto ibscbs = campos::objetoOpcional(raiz, "", caminho);
    if (!ibscbs) {
        return std::unexpected(ibscbs.error());
    }
    if (!ibscbs->has_value()) {
        return std::optional<nb::Ibscbs>{std::nullopt};
    }
    const QJsonObject& objeto = **ibscbs;

    const auto cIndOpTexto = campos::textoOpcional(objeto, caminho, "cIndOp");
    if (!cIndOpTexto) {
        return std::unexpected(cIndOpTexto.error());
    }
    const auto cIndOp = converterOpcional<nb::CodigoIndicadorOperacao>(*cIndOpTexto, &nb::CodigoIndicadorOperacao::de);
    if (!cIndOp) {
        return std::unexpected(cIndOp.error());
    }

    const auto tpOperTexto = campos::textoOpcional(objeto, caminho, "tpOper");
    if (!tpOperTexto) {
        return std::unexpected(tpOperTexto.error());
    }
    const auto tpOper = converterOpcional<nb::TipoOperacao>(*tpOperTexto, &nb::TipoOperacao::de);
    if (!tpOper) {
        return std::unexpected(tpOper.error());
    }

    const auto indDestTexto = campos::textoObrigatorio(objeto, caminho, "indDest");
    if (!indDestTexto) {
        return std::unexpected(indDestTexto.error());
    }
    const auto indDest = nb::IndicadorDestinatario::de(*indDestTexto);
    if (!indDest) {
        return std::unexpected(indDest.error());
    }

    const auto cstTexto = campos::textoObrigatorio(objeto, caminho, "cst");
    if (!cstTexto) {
        return std::unexpected(cstTexto.error());
    }
    const auto cst = nb::CodigoSituacaoTributaria::de(*cstTexto);
    if (!cst) {
        return std::unexpected(cst.error());
    }

    const auto cClassTribTexto = campos::textoObrigatorio(objeto, caminho, "cClassTrib");
    if (!cClassTribTexto) {
        return std::unexpected(cClassTribTexto.error());
    }
    const auto cClassTrib = nb::CodigoClassificacaoTributaria::de(*cClassTribTexto);
    if (!cClassTrib) {
        return std::unexpected(cClassTrib.error());
    }

    const nb::TributacaoIbscbs trib = nb::TributacaoIbscbs::montar(*cst, *cClassTrib);
    const auto montado = nb::Ibscbs::montar(*cIndOp, *tpOper, std::nullopt, *indDest, trib);
    if (!montado) {
        return std::unexpected(montado.error());
    }
    return std::optional<nb::Ibscbs>{*montado};
}

} // namespace nl::mid::composicao::leitores
