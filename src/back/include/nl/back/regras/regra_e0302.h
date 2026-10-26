// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regra E0302 (NL-1.5): cLocPrestacao existe na TAB.LOC.GERAL (municípios do IBGE e 0000000, Águas Marítimas), consultada no port. Linha Excel 333 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026.
#ifndef NL_BACK_REGRAS_REGRA_E0302_H
#define NL_BACK_REGRAS_REGRA_E0302_H

#include <nl/back/regras/regra_local.h>
#include <nl/back/ports/catalogo_localidades.h>

namespace nl::back {

class RegraE0302 final : public RegraLocal
{
public:
    /// O catálogo é guardado por referência; quem constrói mantém o catálogo vivo.
    explicit RegraE0302(const CatalogoLocalidades& catalogo) : catalogo_{catalogo} {}

    std::string_view codigo() const noexcept override;
    std::optional<nl::base::Erro> verificar(const Dps& dps) const override;

private:
    const CatalogoLocalidades& catalogo_;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_E0302_H
