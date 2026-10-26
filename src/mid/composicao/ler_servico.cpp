// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do serviço (NL-1.7): cLocPrestacao, cTribNac, xDescServ obrigatórios e cNBS opcional.
#include "campos_json.h"
#include "leitores_comuns.h"
#include "leitores_dps.h"

#include <nl/back/dps/codigo_nbs.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/descricao_servico.h>
#include <nl/back/identificacao/codigo_municipio.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

std::expected<nb::Servico, nl::base::Erro> lerServico(const QJsonObject& raiz)
{
    constexpr std::string_view caminho = "servico";
    const auto servico = campos::objetoObrigatorio(raiz, "", caminho);
    if (!servico) {
        return std::unexpected(servico.error());
    }

    const auto cLocPrestacaoTexto = campos::textoObrigatorio(*servico, caminho, "cLocPrestacao");
    if (!cLocPrestacaoTexto) {
        return std::unexpected(cLocPrestacaoTexto.error());
    }
    const auto cLocPrestacao = nb::CodigoMunicipio::de(*cLocPrestacaoTexto);
    if (!cLocPrestacao) {
        return std::unexpected(cLocPrestacao.error());
    }

    const auto cTribNacTexto = campos::textoObrigatorio(*servico, caminho, "cTribNac");
    if (!cTribNacTexto) {
        return std::unexpected(cTribNacTexto.error());
    }
    const auto cTribNac = nb::CodigoTribNacional::de(*cTribNacTexto);
    if (!cTribNac) {
        return std::unexpected(cTribNac.error());
    }

    const auto xDescServTexto = campos::textoObrigatorio(*servico, caminho, "xDescServ");
    if (!xDescServTexto) {
        return std::unexpected(xDescServTexto.error());
    }
    const auto xDescServ = nb::DescricaoServico::de(*xDescServTexto);
    if (!xDescServ) {
        return std::unexpected(xDescServ.error());
    }

    const auto cNbsTexto = campos::textoOpcional(*servico, caminho, "cNbs");
    if (!cNbsTexto) {
        return std::unexpected(cNbsTexto.error());
    }
    const auto cNbs = converterOpcional<nb::CodigoNbs>(*cNbsTexto, &nb::CodigoNbs::de);
    if (!cNbs) {
        return std::unexpected(cNbs.error());
    }

    return nb::Servico::montar(*cLocPrestacao, *cTribNac, *xDescServ, *cNbs);
}

} // namespace nl::mid::composicao::leitores
