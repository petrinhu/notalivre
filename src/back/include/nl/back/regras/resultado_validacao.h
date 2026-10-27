// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resultado do ValidadorLocal (NL-1.5): todos os erros e todos os avisos da DPS, sem parar na primeira violação.
#ifndef NL_BACK_REGRAS_RESULTADO_VALIDACAO_H
#define NL_BACK_REGRAS_RESULTADO_VALIDACAO_H

#include <vector>

#include <nl/base/erro.h>
#include <nl/back/regras/aviso.h>

namespace nl::back {

struct ResultadoValidacao
{
    std::vector<nl::base::Erro> erros;
    std::vector<Aviso> avisos;

    /// Só os erros bloqueiam. Avisos não entram nesta conta.
    bool podeEmitir() const noexcept { return erros.empty(); }
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_RESULTADO_VALIDACAO_H
