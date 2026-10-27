// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de RegApTribSN (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1019-1032 (TSRegimeApuracaoSimpNac); campo regApTribSN em tiposComplexos_v1.01.xsd:978.

#include <nl/back/dps/reg_ap_trib_sn.h>

#include <optional>

namespace nl::back {

std::expected<RegApTribSN, nl::base::Erro> RegApTribSN::de(QStringView entrada)
{
    if (entrada == u"1") {
        return RegApTribSN{RegApTribSN::Valor::federalEMunicipalPeloSN};
    }
    if (entrada == u"2") {
        return RegApTribSN{RegApTribSN::Valor::federalPeloSnIssForaSN};
    }
    if (entrada == u"3") {
        return RegApTribSN{RegApTribSN::Valor::federalEMunicipalForaSN};
    }
    return std::unexpected(nl::base::Erro{"regime.regapttribsn", std::nullopt, "RegApTribSN: valor fora do conjunto do XSD"});
}

} // namespace nl::back
