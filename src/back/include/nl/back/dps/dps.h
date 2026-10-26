// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Entidade Dps (infDPS, NL-1.4): raiz da DPS validada. O Id sai da montagem (IdDps::montar), nunca é recebido pronto (NL-1.3).

#ifndef NL_BACK_DPS_DPS_H
#define NL_BACK_DPS_DPS_H

#include <expected>

#include <nl/base/erro.h>
#include <nl/back/dps/dados_dps.h>
#include <nl/back/dps/dh_emi.h>
#include <nl/back/dps/id_dps.h>


namespace nl::back {

/// Invariante: dhEmi (DhEmi, DA-1.27) e dCompet válidos, Id derivado de cLocEmi, prestador, série e número. Só se constrói por Dps::montar.
class Dps
{
public:
    /// Erros: os de DhEmi::de ("dhemi.formato"), "dps.dcompet" (data inválida) e os de IdDps::montar.
    static std::expected<Dps, nl::base::Erro> montar(DadosDps dados);

    /// tpEmit = 1 por construção (DA-1.3): a v1 emite só como prestador. Não há setter.
    static constexpr int tpEmit() noexcept { return 1; }

    /// finNFSe = 0 (NFS-e regular), por construção (TSRTCFinNFSe, tiposSimples_v1.01.xsd:1682-1690).
    static constexpr int finNFSe() noexcept { return 0; }

    const IdDps& id() const noexcept { return id_; }
    /// dhEmi já validado e sem milissegundos (DA-1.27). dados().dhEmi guarda a entrada, como veio.
    const DhEmi& dhEmi() const noexcept { return dhEmi_; }
    const DadosDps& dados() const noexcept { return dados_; }

private:
    Dps(IdDps id, DhEmi dhEmi, DadosDps dados) : id_{std::move(id)}, dhEmi_{std::move(dhEmi)}, dados_{std::move(dados)} {}

    IdDps id_;
    DhEmi dhEmi_;
    DadosDps dados_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_DPS_H
