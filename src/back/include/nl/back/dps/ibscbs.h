// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupo IBS/CBS da DPS (infDPS/IBSCBS, NL-1.4, TCRTCInfoIBSCBS, tiposComplexos_v1.01.xsd:2322-2355). Opcional em 2026 (spec 4.11).

#ifndef NL_BACK_DPS_IBSCBS_H
#define NL_BACK_DPS_IBSCBS_H

#include <expected>
#include <optional>
#include <utility>

#include <nl/base/erro.h>
#include <nl/back/dps/codigo_indicador_operacao.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/ref_nfse.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/tributacao_ibscbs.h>


namespace nl::back {

/// Invariantes (DA-1.5, aba RN de 20/10): gRefNFSe é obrigatório com tpOper 2 ou 3 (E0905, linha Excel 576) e proibido
/// sem tpOper ou com tpOper 1, 4 ou 5 (E0906, linha Excel 577). Só se constrói por Ibscbs::montar.
class Ibscbs
{
public:
    /// Erros: "ibscbs.gref_obrigatorio" (tpOper 2 ou 3 sem gRefNFSe) e "ibscbs.gref_proibido" (gRefNFSe fora desses casos).
    static std::expected<Ibscbs, nl::base::Erro> montar(std::optional<CodigoIndicadorOperacao> cIndOp, std::optional<TipoOperacao> tpOper,
                                                        std::optional<RefNfse> gRefNFSe, IndicadorDestinatario indDest, TributacaoIbscbs trib);

    const std::optional<CodigoIndicadorOperacao>& cIndOp() const noexcept { return cIndOp_; }
    const std::optional<TipoOperacao>& tpOper() const noexcept { return tpOper_; }
    const std::optional<RefNfse>& gRefNFSe() const noexcept { return gRefNFSe_; }
    IndicadorDestinatario indDest() const noexcept { return indDest_; }
    const TributacaoIbscbs& trib() const noexcept { return trib_; }

    bool operator==(const Ibscbs& outro) const = default;

private:
    Ibscbs(std::optional<CodigoIndicadorOperacao> cIndOp, std::optional<TipoOperacao> tpOper, std::optional<RefNfse> gRefNFSe,
           IndicadorDestinatario indDest, TributacaoIbscbs trib)
        : cIndOp_{std::move(cIndOp)}, tpOper_{std::move(tpOper)}, gRefNFSe_{std::move(gRefNFSe)}, indDest_{indDest}, trib_{std::move(trib)}
    {
    }

    std::optional<CodigoIndicadorOperacao> cIndOp_;
    std::optional<TipoOperacao> tpOper_;
    std::optional<RefNfse> gRefNFSe_;
    IndicadorDestinatario indDest_;
    TributacaoIbscbs trib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_IBSCBS_H
