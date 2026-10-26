// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Interface RegraAviso (NL-1.5, ponto L10): regra só do site, que devolve aviso e nunca erro.
#ifndef NL_BACK_REGRAS_REGRA_AVISO_H
#define NL_BACK_REGRAS_REGRA_AVISO_H

#include <optional>
#include <string_view>

#include <nl/back/dps/dps.h>
#include <nl/back/regras/aviso.h>

namespace nl::back {

class RegraAviso
{
public:
    virtual ~RegraAviso();

    /// Código D no formato do plano (por exemplo "D1").
    virtual std::string_view codigo() const noexcept = 0;

    /// Aviso quando a DPS cai na condição do site, nullopt caso contrário. Nunca bloqueia.
    virtual std::optional<Aviso> avaliar(const Dps& dps) const = 0;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_REGRA_AVISO_H
