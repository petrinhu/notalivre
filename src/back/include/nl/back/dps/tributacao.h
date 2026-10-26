// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tributação do serviço (grupo infDPS/valores/trib): tribMun e totTrib obrigatórios (NL-1.4, TCInfoTributacao, tiposComplexos_v1.01.xsd:1775).

#ifndef NL_BACK_DPS_TRIBUTACAO_H
#define NL_BACK_DPS_TRIBUTACAO_H

#include <utility>

#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/dps/total_tributos.h>


namespace nl::back {

class Tributacao
{
public:
    /// tribFed (PIS, COFINS, IRRF, CSLL, 0-1) não entra na v1: candidato à INBOX, não é corte silencioso (L-21).
    static Tributacao montar(TributacaoMunicipal tribMun, TotalTributos totTrib)
    {
        return Tributacao{std::move(tribMun), std::move(totTrib)};
    }

    const TributacaoMunicipal& tribMun() const noexcept { return tribMun_; }
    const TotalTributos& totTrib() const noexcept { return totTrib_; }

    bool operator==(const Tributacao& outro) const = default;

private:
    Tributacao(TributacaoMunicipal tribMun, TotalTributos totTrib) : tribMun_{tribMun}, totTrib_{std::move(totTrib)} {}

    TributacaoMunicipal tribMun_;
    TotalTributos totTrib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIBUTACAO_H
