// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Total aproximado de tributos (grupo totTrib, obrigatório): escolha entre quatro alternativas de TCTribTotal (tiposComplexos_v1.01.xsd:2233-2240).

#ifndef NL_BACK_DPS_TOTAL_TRIBUTOS_H
#define NL_BACK_DPS_TOTAL_TRIBUTOS_H

#include <variant>

#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>


namespace nl::back {

/// Escolha do XSD: vTotTrib, pTotTrib, indTotTrib ou pTotTribSN. Qual o app usa é decisão de produto, não do domínio.
using TotalTributos = std::variant<TributosMonetarios, TributosPercentuais, IndicadorTotalTributos, PercentualTributoSn>;

} // namespace nl::back

#endif // NL_BACK_DPS_TOTAL_TRIBUTOS_H
