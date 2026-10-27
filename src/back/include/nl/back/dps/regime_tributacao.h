// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regime de tributação do prestador: opSimpNac, regApTribSN e regEspTrib (NL-1.4, TCRegTrib, tiposComplexos_v1.01.xsd:966).

#ifndef NL_BACK_DPS_REGIME_TRIBUTACAO_H
#define NL_BACK_DPS_REGIME_TRIBUTACAO_H

#include <expected>
#include <optional>
#include <string>
#include <utility>

#include <nl/base/erro.h>
#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/reg_ap_trib_sn.h>
#include <nl/back/dps/reg_esp_trib.h>


namespace nl::back {

/// Regime do prestador. regApTribSN só existe com opSimpNac 3 (DA-1.16). E0162 e E0166 (MEI e ME/EPP) são da NL-1.5.
/// Invariante: os três campos já vêm validados pelos próprios tipos; regApTribSN é opcional no XSD (tiposComplexos_v1.01.xsd:978).
class RegimeTributacao
{
public:
    /// Erro: "regime.regapttribsn_sem_meepp" (regApTribSN presente com opSimpNac diferente de 3, DA-1.16).
    static std::expected<RegimeTributacao, nl::base::Erro> montar(OpSimpNac opSimpNac, std::optional<RegApTribSN> regApTribSN,
                                                                  RegEspTrib regEspTrib)
    {
        if (regApTribSN && opSimpNac.valor() != OpSimpNac::Valor::meEpp) {
            return std::unexpected(nl::base::Erro{"regime.regapttribsn_sem_meepp", std::string{"prest/regTrib/regApTribSN"},
                                                  "regApTribSN só pode existir com opSimpNac 3 (ME/EPP)"});
        }
        return RegimeTributacao{opSimpNac, std::move(regApTribSN), regEspTrib};
    }

    const OpSimpNac& opSimpNac() const noexcept { return opSimpNac_; }
    const std::optional<RegApTribSN>& regApTribSN() const noexcept { return regApTribSN_; }
    RegEspTrib regEspTrib() const noexcept { return regEspTrib_; }

    bool operator==(const RegimeTributacao& outro) const = default;

private:
    RegimeTributacao(OpSimpNac opSimpNac, std::optional<RegApTribSN> regApTribSN, RegEspTrib regEspTrib)
        : opSimpNac_{opSimpNac}, regApTribSN_{std::move(regApTribSN)}, regEspTrib_{regEspTrib}
    {
    }

    OpSimpNac opSimpNac_;
    std::optional<RegApTribSN> regApTribSN_;
    RegEspTrib regEspTrib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_REGIME_TRIBUTACAO_H
