// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Valores do serviço (grupo infDPS/valores): vServ e trib obrigatórios (NL-1.4, TCInfoValores, tiposComplexos_v1.01.xsd:1742).

#ifndef NL_BACK_DPS_VALORES_SERVICO_H
#define NL_BACK_DPS_VALORES_SERVICO_H

#include <utility>

#include <nl/back/dinheiro/valor.h>
#include <nl/back/dps/tributacao.h>


namespace nl::back {

class ValoresServico
{
public:
    /// vReceb, vDescCondIncond e vAjusteBC ficam fora da v1 (vAjusteBC é a linha 2 da INBOX).
    static ValoresServico montar(Valor vServ, Tributacao trib)
    {
        return ValoresServico{vServ, std::move(trib)};
    }

    const Valor& vServ() const noexcept { return vServ_; }
    const Tributacao& trib() const noexcept { return trib_; }

    bool operator==(const ValoresServico& outro) const = default;

private:
    ValoresServico(Valor vServ, Tributacao trib) : vServ_{vServ}, trib_{std::move(trib)} {}

    Valor vServ_;
    Tributacao trib_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_VALORES_SERVICO_H
