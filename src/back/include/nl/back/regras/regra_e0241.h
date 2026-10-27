// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0241 (NL-1.5): com retenção pelo tomador, o tomador é CNPJ ou CPF e tem endereço nacional (ramo da retenção; o ramo da incidência é Onda 5). Linha Excel 278 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026.
#ifndef NL_BACK_REGRAS_REGRA_E0241_H
#define NL_BACK_REGRAS_REGRA_E0241_H

#include <nl/back/regras/regra_local.h>

namespace nl::back {

class RegraE0241 final : public RegraLocal
{
public:
    std::string_view codigo() const noexcept override;
    std::optional<nl::base::Erro> verificar(const Dps& dps) const override;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_E0241_H
