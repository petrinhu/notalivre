// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0313 (NL-2.10h, H1): não é permitido informar cTribMun quando o emitente é MEI (opSimpNac 2). Linha Excel 340 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026.
#ifndef NL_BACK_REGRAS_REGRA_E0313_H
#define NL_BACK_REGRAS_REGRA_E0313_H

#include <nl/back/regras/regra_local.h>

namespace nl::back {

class RegraE0313 final : public RegraLocal
{
public:
    std::string_view codigo() const noexcept override;
    std::optional<nl::base::Erro> verificar(const Dps& dps) const override;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_E0313_H
