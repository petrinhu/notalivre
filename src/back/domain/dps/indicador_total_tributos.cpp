// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de IndicadorTotalTributos (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1268 (TSTipoIndTotTrib).

#include <nl/back/dps/indicador_total_tributos.h>

#include <optional>

namespace nl::back {

std::expected<IndicadorTotalTributos, nl::base::Erro> IndicadorTotalTributos::de(QStringView entrada)
{
    if (entrada == u"0") {
        return IndicadorTotalTributos{IndicadorTotalTributos::Valor::naoInformar};
    }
    return std::unexpected(nl::base::Erro{"indtottrib.valor", std::nullopt, "IndicadorTotalTributos: valor fora do conjunto do XSD"});
}

} // namespace nl::back
