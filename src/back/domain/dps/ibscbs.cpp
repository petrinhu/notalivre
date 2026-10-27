// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de Ibscbs (NL-1.4, DA-1.5). E0905 e E0906 da aba RN de 20/10 (linhas Excel 576 e 577).

#include <nl/back/dps/ibscbs.h>

#include <utility>

namespace nl::back {
namespace {

/// gRefNFSe é obrigatório com tpOper 2 ou 3 (E0905) e proibido sem tpOper ou com 1, 4 ou 5 (E0906).
bool exigeReferencia(const std::optional<TipoOperacao>& tpOper)
{
    if (!tpOper) {
        return false;
    }
    const TipoOperacao::Valor valor = tpOper->valor();
    return valor == TipoOperacao::Valor::recebimentoComFornecimentoRealizado
           || valor == TipoOperacao::Valor::fornecimentoComPagamentoRealizado;
}

} // namespace

std::expected<Ibscbs, nl::base::Erro> Ibscbs::montar(std::optional<CodigoIndicadorOperacao> cIndOp, std::optional<TipoOperacao> tpOper,
                                                     std::optional<RefNfse> gRefNFSe, IndicadorDestinatario indDest,
                                                     TributacaoIbscbs trib)
{
    const bool exige = exigeReferencia(tpOper);
    if (exige && !gRefNFSe) {
        return std::unexpected(nl::base::Erro{"E0905", std::string{"IBSCBS/gRefNFSe"},
                                              "tpOper 2 ou 3 exige o grupo gRefNFSe com a NFS-e referenciada"});
    }
    if (!exige && gRefNFSe) {
        return std::unexpected(nl::base::Erro{"E0906", std::string{"IBSCBS/gRefNFSe"},
                                              "gRefNFSe só pode existir com tpOper 2 ou 3"});
    }
    return Ibscbs{std::move(cIndOp), std::move(tpOper), std::move(gRefNFSe), indDest, std::move(trib)};
}

} // namespace nl::back
