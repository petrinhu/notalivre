// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Dados da NFS-e substituída (grupo infDPS/subst): chave, código de justificativa e motivo opcional (NL-1.4, TCSubstituicao, tiposComplexos_v1.01.xsd:862).

#ifndef NL_BACK_DPS_SUBSTITUICAO_H
#define NL_BACK_DPS_SUBSTITUICAO_H

#include <optional>
#include <utility>

#include <nl/back/dps/chave_nfse.h>
#include <nl/back/dps/codigo_justificativa_substituicao.h>
#include <nl/back/dps/texto_limitado.h>


namespace nl::back {

/// Motivo, 15 a 255 caracteres (TSMotivo, tiposSimples_v1.01.xsd:348).
using MotivoSubstituicao = TextoLimitado<15, 255>;

/// Invariante: a chave e o código já vêm validados. cMotivo tem exatamente duas posições (CodigoJustificativaSubstituicao).
class Substituicao
{
public:
    static Substituicao montar(ChaveNfse chSubstda, CodigoJustificativaSubstituicao cMotivo, std::optional<MotivoSubstituicao> xMotivo)
    {
        return Substituicao{std::move(chSubstda), std::move(cMotivo), std::move(xMotivo)};
    }

    const ChaveNfse& chSubstda() const noexcept { return chSubstda_; }
    const CodigoJustificativaSubstituicao& cMotivo() const noexcept { return cMotivo_; }
    const std::optional<MotivoSubstituicao>& xMotivo() const noexcept { return xMotivo_; }

    bool operator==(const Substituicao& outro) const = default;

private:
    Substituicao(ChaveNfse chSubstda, CodigoJustificativaSubstituicao cMotivo, std::optional<MotivoSubstituicao> xMotivo)
        : chSubstda_{std::move(chSubstda)}, cMotivo_{std::move(cMotivo)}, xMotivo_{std::move(xMotivo)}
    {
    }

    ChaveNfse chSubstda_;
    CodigoJustificativaSubstituicao cMotivo_;
    std::optional<MotivoSubstituicao> xMotivo_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_SUBSTITUICAO_H
