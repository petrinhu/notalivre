// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0202 (NL-1.5): CNPJ do tomador igual ao CNPJ do prestador, comparado por inteiro. Linha Excel 259 da aba RN DPS_NFS-e 1.01 do Anexo I de 24/11/2026 (era 257 no de 20/10).
#ifndef NL_BACK_REGRAS_REGRA_E0202_H
#define NL_BACK_REGRAS_REGRA_E0202_H

#include <nl/back/regras/regra_local.h>

namespace nl::back {

class RegraE0202 final : public RegraLocal
{
public:
    std::string_view codigo() const noexcept override;
    std::optional<nl::base::Erro> verificar(const Dps& dps) const override;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_E0202_H
