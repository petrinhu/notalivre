// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regime do totTrib (NL-2.10j, J1): qual forma de totTrib cada regime do Simples aceita. E0712 (ME/EPP sem indTotTrib), E0713 (não optante sem indTotTrib e sem pTotTribSN), E0710 (MEI sem pTotTribSN). Linhas Excel 562, 563 e 565 do Anexo I de 20/10/2026. Regra pura, sem I/O.

#ifndef NL_BACK_DPS_TOTAL_TRIBUTOS_REGIME_H
#define NL_BACK_DPS_TOTAL_TRIBUTOS_REGIME_H

#include <optional>

#include <nl/base/erro.h>
#include <nl/back/dps/dados_dps.h>

namespace nl::back {

/// Recusa a forma de totTrib que o regime do prestador não aceita. Erro: "dps.tottrib_regime", com a regra no texto (E0712, E0713 ou E0710).
std::optional<nl::base::Erro> verificarTotTribRegime(const DadosDps& dados);

} // namespace nl::back

#endif // NL_BACK_DPS_TOTAL_TRIBUTOS_REGIME_H
