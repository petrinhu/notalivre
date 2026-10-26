// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Serviço prestado (grupo infDPS/serv): local da prestação, código de tributação nacional e descrição (NL-1.4, TCServ, tiposComplexos_v1.01.xsd:1412).

#ifndef NL_BACK_DPS_SERVICO_H
#define NL_BACK_DPS_SERVICO_H

#include <optional>
#include <utility>

#include <nl/back/dps/codigo_nbs.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/descricao_servico.h>
#include <nl/back/identificacao/codigo_municipio.h>


namespace nl::back {

class Servico
{
public:
    /// Invariante: os três campos já vêm validados. Sem cLocPrestacao de país (TCLocPrest, escolha): fora da v1, INBOX.
    static Servico montar(CodigoMunicipio cLocPrestacao, CodigoTribNacional cTribNac, DescricaoServico xDescServ,
                          std::optional<CodigoNbs> cNBS)
    {
        return Servico{std::move(cLocPrestacao), std::move(cTribNac), std::move(xDescServ), std::move(cNBS)};
    }

    const std::optional<CodigoNbs>& cNBS() const noexcept { return cNBS_; }

    const CodigoMunicipio& cLocPrestacao() const noexcept { return cLocPrestacao_; }
    const CodigoTribNacional& cTribNac() const noexcept { return cTribNac_; }
    const DescricaoServico& xDescServ() const noexcept { return xDescServ_; }

    bool operator==(const Servico& outro) const = default;

private:
    Servico(CodigoMunicipio cLocPrestacao, CodigoTribNacional cTribNac, DescricaoServico xDescServ, std::optional<CodigoNbs> cNBS)
        : cLocPrestacao_{std::move(cLocPrestacao)}, cTribNac_{std::move(cTribNac)}, xDescServ_{std::move(xDescServ)}, cNBS_{std::move(cNBS)}
    {
    }

    CodigoMunicipio cLocPrestacao_;
    CodigoTribNacional cTribNac_;
    DescricaoServico xDescServ_;
    std::optional<CodigoNbs> cNBS_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_SERVICO_H
