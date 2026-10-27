// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Totais aproximados de tributos em reais: vTotTribFed, vTotTribEst e vTotTribMun (alternativa de TCTribTotal, tiposComplexos_v1.01.xsd:2233).

#ifndef NL_BACK_DPS_TRIBUTOS_MONETARIOS_H
#define NL_BACK_DPS_TRIBUTOS_MONETARIOS_H

#include <utility>

#include <nl/back/dinheiro/valor.h>


namespace nl::back {

class TributosMonetarios
{
public:
    static TributosMonetarios montar(Valor vTotTribFed, Valor vTotTribEst, Valor vTotTribMun)
    {
        return TributosMonetarios{vTotTribFed, vTotTribEst, vTotTribMun};
    }

    const Valor& vTotTribFed() const noexcept { return vTotTribFed_; }
    const Valor& vTotTribEst() const noexcept { return vTotTribEst_; }
    const Valor& vTotTribMun() const noexcept { return vTotTribMun_; }

    bool operator==(const TributosMonetarios& outro) const = default;

private:
    TributosMonetarios(Valor vTotTribFed, Valor vTotTribEst, Valor vTotTribMun)
        : vTotTribFed_{vTotTribFed}, vTotTribEst_{vTotTribEst}, vTotTribMun_{vTotTribMun}
    {
    }

    Valor vTotTribFed_;
    Valor vTotTribEst_;
    Valor vTotTribMun_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIBUTOS_MONETARIOS_H
