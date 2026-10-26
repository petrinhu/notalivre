// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de OpSimpNac (NL-1.4). Fonte: tiposComplexos_v1.01.xsd:968 e tiposSimples_v1.01.xsd:1003-1016 (TSOpSimpNac).

#include <nl/back/dps/op_simp_nac.h>

#include <optional>

namespace nl::back {

std::expected<OpSimpNac, nl::base::Erro> OpSimpNac::de(QStringView entrada)
{
    if (entrada == u"1") {
        return OpSimpNac{OpSimpNac::Valor::naoOptante};
    }
    if (entrada == u"2") {
        return OpSimpNac{OpSimpNac::Valor::mei};
    }
    if (entrada == u"3") {
        return OpSimpNac{OpSimpNac::Valor::meEpp};
    }
    return std::unexpected(nl::base::Erro{"regime.opsimpnac", std::nullopt, "OpSimpNac: valor fora do conjunto do XSD"});
}

} // namespace nl::back
