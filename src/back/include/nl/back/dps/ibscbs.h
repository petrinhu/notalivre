// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupo IBS/CBS da DPS (infDPS/IBSCBS, NL-1.4, TCRTCInfoIBSCBS, tiposComplexos_v1.01.xsd:2392-2459 do leiaute de 24/11/2026).
// Opcional em 2026 (spec 4.11). indDest saiu do grupo e foi para a raiz de infDPS (DadosDps, NL-3.L).

#ifndef NL_BACK_DPS_IBSCBS_H
#define NL_BACK_DPS_IBSCBS_H

#include <expected>
#include <optional>
#include <utility>

#include <nl/base/erro.h>
#include <nl/back/dps/codigo_indicador_operacao.h>
#include <nl/back/dps/ref_nfse.h>
#include <nl/back/dps/tipo_operacao.h>
#include <nl/back/dps/tributacao_ibscbs.h>


namespace nl::back {

/// Invariantes (DA-1.5, aba RN do Anexo I de 24/11/2026): gRefNFSe é obrigatório com tpOper 2 ou 3 (E0905, linha Excel 610)
/// e proibido sem tpOper ou com tpOper 1, 4 ou 5 (E0906, linha Excel 611). Só se constrói por Ibscbs::montar.
class Ibscbs
{
public:
    /// Erros: "E0905" (tpOper 2 ou 3 sem gRefNFSe) e "E0906" (gRefNFSe fora desses casos).
    static std::expected<Ibscbs, nl::base::Erro> montar(std::optional<CodigoIndicadorOperacao> cIndOp, std::optional<TipoOperacao> tpOper,
                                                        std::optional<RefNfse> gRefNFSe, TributacaoIbscbs trib);

    const std::optional<CodigoIndicadorOperacao>& cIndOp() const noexcept { return cIndOp_; }
    const std::optional<TipoOperacao>& tpOper() const noexcept { return tpOper_; }
    const std::optional<RefNfse>& gRefNFSe() const noexcept { return gRefNFSe_; }
    const TributacaoIbscbs& trib() const noexcept { return trib_; }

    bool operator==(const Ibscbs& outro) const = default;

private:
    Ibscbs(std::optional<CodigoIndicadorOperacao> cIndOp, std::optional<TipoOperacao> tpOper, std::optional<RefNfse> gRefNFSe,
           TributacaoIbscbs trib)
        : cIndOp_{std::move(cIndOp)}, tpOper_{std::move(tpOper)}, gRefNFSe_{std::move(gRefNFSe)}, trib_{std::move(trib)}
    {
    }

    std::optional<CodigoIndicadorOperacao> cIndOp_;
    std::optional<TipoOperacao> tpOper_;
    std::optional<RefNfse> gRefNFSe_;
    TributacaoIbscbs trib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_IBSCBS_H
