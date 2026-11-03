// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Faixas do totTrib (NL-2.10j, J1): vTotTrib entre 0 e o valor do serviço (E0702 a E0704, linhas Excel 555 a 557); pTotTrib entre 0 e 100% (E0706 a E0708, linhas Excel 559 a 561). pTotTribSN já tem a faixa do tipo (TSDec2V2). Regra pura, sem I/O.

#ifndef NL_BACK_DPS_TOTAL_TRIBUTOS_FAIXA_H
#define NL_BACK_DPS_TOTAL_TRIBUTOS_FAIXA_H

#include <optional>

#include <nl/base/erro.h>
#include <nl/back/dps/dados_dps.h>

namespace nl::back {

/// Recusa o valor ou o percentual de tributo fora da faixa. Erro: "dps.tottrib_faixa", com a regra no texto (E0702 a E0708).
std::optional<nl::base::Erro> verificarTotTribFaixa(const DadosDps& dados);

} // namespace nl::back

#endif // NL_BACK_DPS_TOTAL_TRIBUTOS_FAIXA_H
