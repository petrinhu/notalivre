// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tributação do ISSQN (grupo infDPS/valores/trib/tribMun): tribISSQN e tpRetISSQN obrigatórios; pAliq 0-1 (NL-1.4, TCTribMunicipal, tiposComplexos_v1.01.xsd:1972).

#ifndef NL_BACK_DPS_TRIBUTACAO_MUNICIPAL_H
#define NL_BACK_DPS_TRIBUTACAO_MUNICIPAL_H

#include <expected>
#include <optional>
#include <string>
#include <utility>

#include <nl/base/erro.h>
#include <nl/back/dps/aliquota_issqn.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/trib_issqn.h>


namespace nl::back {

class TributacaoMunicipal
{
public:
    /// Erro: "dps.nao_suportado_v1" para tribISSQN 2 (imunidade) e 3 (exportação), até o produto decidir (DA-1.17).
    static std::expected<TributacaoMunicipal, nl::base::Erro> montar(TribIssqn tribISSQN, RetencaoIssqn tpRetISSQN,
                                                                     std::optional<AliquotaIssqn> pAliq)
    {
        if (tribISSQN.valor() == TribIssqn::Valor::imunidade || tribISSQN.valor() == TribIssqn::Valor::exportacaoServico) {
            return std::unexpected(nl::base::Erro{"dps.nao_suportado_v1", std::string{"prest/tribISSQN"},
                                                  "tribISSQN 2 (imunidade) e 3 (exportação) não são suportados na v1"});
        }
        return TributacaoMunicipal{tribISSQN, tpRetISSQN, std::move(pAliq)};
    }

    const TribIssqn& tribISSQN() const noexcept { return tribISSQN_; }
    const RetencaoIssqn& tpRetISSQN() const noexcept { return tpRetISSQN_; }
    const std::optional<AliquotaIssqn>& pAliq() const noexcept { return pAliq_; }

    bool operator==(const TributacaoMunicipal& outro) const = default;

private:
    TributacaoMunicipal(TribIssqn tribISSQN, RetencaoIssqn tpRetISSQN, std::optional<AliquotaIssqn> pAliq)
        : tribISSQN_{tribISSQN}, tpRetISSQN_{tpRetISSQN}, pAliq_{std::move(pAliq)}
    {
    }

    TribIssqn tribISSQN_;
    RetencaoIssqn tpRetISSQN_;
    std::optional<AliquotaIssqn> pAliq_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIBUTACAO_MUNICIPAL_H
