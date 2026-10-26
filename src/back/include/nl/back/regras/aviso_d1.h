// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Aviso D1 (NL-1.5, ponto L10): tomador identificado por CNPJ sem endereço nacional. Regra só do site: aviso, nunca erro.
// A linha 276 da aba RN (E0235) tem K e N = X, então a Sefin não a aplica à DPS do app (DA-1.19).
#ifndef NL_BACK_REGRAS_AVISO_D1_H
#define NL_BACK_REGRAS_AVISO_D1_H

#include <nl/back/regras/regra_aviso.h>

namespace nl::back {

class AvisoD1 final : public RegraAviso
{
public:
    std::string_view codigo() const noexcept override;
    std::optional<Aviso> avaliar(const Dps& dps) const override;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_AVISO_D1_H
