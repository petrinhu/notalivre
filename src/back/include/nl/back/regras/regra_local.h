// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Interface RegraLocal (NL-1.5): uma regra E que dá para checar sem a Sefin. Uma classe por código (L-34).
#ifndef NL_BACK_REGRAS_REGRA_LOCAL_H
#define NL_BACK_REGRAS_REGRA_LOCAL_H

#include <optional>
#include <string_view>

#include <nl/base/erro.h>
#include <nl/back/dps/dps.h>

namespace nl::back {

class RegraLocal
{
public:
    virtual ~RegraLocal();

    /// Código E no formato do XLSX de 20/10 (por exemplo "E0202"). É o código do erro devolvido.
    virtual std::string_view codigo() const noexcept = 0;

    /// Primeira violação da regra, ou nullopt se a DPS a cumpre. Função pura: não toca I/O nem estado global.
    virtual std::optional<nl::base::Erro> verificar(const Dps& dps) const = 0;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_LOCAL_H
