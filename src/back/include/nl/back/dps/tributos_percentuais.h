// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Percentuais aproximados de tributos: pTotTribFed, pTotTribEst e pTotTribMun (alternativa de TCTribTotal, tiposComplexos_v1.01.xsd:2233).

#ifndef NL_BACK_DPS_TRIBUTOS_PERCENTUAIS_H
#define NL_BACK_DPS_TRIBUTOS_PERCENTUAIS_H

#include <utility>

#include <nl/back/dps/percentual_tributo.h>


namespace nl::back {

class TributosPercentuais
{
public:
    static TributosPercentuais montar(PercentualTributo pTotTribFed, PercentualTributo pTotTribEst, PercentualTributo pTotTribMun)
    {
        return TributosPercentuais{pTotTribFed, pTotTribEst, pTotTribMun};
    }

    const PercentualTributo& pTotTribFed() const noexcept { return pTotTribFed_; }
    const PercentualTributo& pTotTribEst() const noexcept { return pTotTribEst_; }
    const PercentualTributo& pTotTribMun() const noexcept { return pTotTribMun_; }

    bool operator==(const TributosPercentuais& outro) const = default;

private:
    TributosPercentuais(PercentualTributo pTotTribFed, PercentualTributo pTotTribEst, PercentualTributo pTotTribMun)
        : pTotTribFed_{std::move(pTotTribFed)}, pTotTribEst_{std::move(pTotTribEst)}, pTotTribMun_{std::move(pTotTribMun)}
    {
    }

    PercentualTributo pTotTribFed_;
    PercentualTributo pTotTribEst_;
    PercentualTributo pTotTribMun_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIBUTOS_PERCENTUAIS_H
