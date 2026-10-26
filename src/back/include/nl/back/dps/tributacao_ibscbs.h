// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tributação do IBS e da CBS (grupo IBSCBS/valores/trib): CST e cClassTrib obrigatórios, só formato (NL-1.4, TCRTCInfoTribIBSCBS, tiposComplexos_v1.01.xsd:2533).

#ifndef NL_BACK_DPS_TRIBUTACAO_IBSCBS_H
#define NL_BACK_DPS_TRIBUTACAO_IBSCBS_H

#include <utility>

#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>


namespace nl::back {

class TributacaoIbscbs
{
public:
    /// gIBSCBS (0-1, com cCredPres, gTribRegular e gDif) fica fora da v1: candidato à INBOX.
    static TributacaoIbscbs montar(CodigoSituacaoTributaria cst, CodigoClassificacaoTributaria cClassTrib)
    {
        return TributacaoIbscbs{std::move(cst), std::move(cClassTrib)};
    }

    const CodigoSituacaoTributaria& cst() const noexcept { return cst_; }
    const CodigoClassificacaoTributaria& cClassTrib() const noexcept { return cClassTrib_; }

    bool operator==(const TributacaoIbscbs& outro) const = default;

private:
    TributacaoIbscbs(CodigoSituacaoTributaria cst, CodigoClassificacaoTributaria cClassTrib)
        : cst_{std::move(cst)}, cClassTrib_{std::move(cClassTrib)}
    {
    }

    CodigoSituacaoTributaria cst_;
    CodigoClassificacaoTributaria cClassTrib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIBUTACAO_IBSCBS_H
