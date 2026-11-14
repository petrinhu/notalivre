// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classe da falha de envio (NL-3.C, plano seção 1.5). Ver classe_falha_envio.h. Corpo provisório: os testes ainda devem ver vermelho.
#include <nl/mid/composicao/classe_falha_envio.h>

namespace nl::mid::composicao {

std::optional<ClasseFalhaEnvio> classeDaFalha(const nl::back::FalhaSefin& /*falha*/) noexcept
{
    return ClasseFalhaEnvio::RespostaSemCodigo;
}

} // namespace nl::mid::composicao
