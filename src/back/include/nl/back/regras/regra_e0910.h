// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0910 (NL-3.L): indDest = 1 exige o grupo dest do destinatário (grupo ausente no modelo: rejeita, como a E0390 faz com
// atvEvento). Linha Excel 300 da aba RN DPS_NFS-e 1.01 do Anexo I de 24/11/2026. A coluna K está vazia; vale a leitura conservadora
// do ponto 5 ratificado pelo líder em 27/11/2026 ("Como rejeição ativa (Recomendado)"). O grupo dest é a pendência NL-I.065.
#ifndef NL_BACK_REGRAS_REGRA_E0910_H
#define NL_BACK_REGRAS_REGRA_E0910_H

#include <nl/back/regras/regra_local.h>

namespace nl::back {

class RegraE0910 final : public RegraLocal
{
public:
    std::string_view codigo() const noexcept override;
    std::optional<nl::base::Erro> verificar(const Dps& dps) const override;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_E0910_H
