// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classe da falha de envio (NL-3.C, plano seção 1.5): a regra motivo x causa x código E que escolhe a classe do catálogo.
// Nenhum texto livre entra aqui: só o motivo e a causa tipados.
#ifndef NL_MID_COMPOSICAO_CLASSE_FALHA_ENVIO_H
#define NL_MID_COMPOSICAO_CLASSE_FALHA_ENVIO_H

#include <nl/back/ports/gateway_sefin.h>
#include <nl/mid/composicao/catalogo_falha_envio.h>

#include <optional>

namespace nl::mid::composicao {

/// Classe da falha, ou nada quando o caminho antigo (sefin.<E>, sefin.dps_duplicada) ainda decide a saída.
[[nodiscard]] std::optional<ClasseFalhaEnvio> classeDaFalha(const nl::back::FalhaSefin& falha) noexcept;

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_CLASSE_FALHA_ENVIO_H
