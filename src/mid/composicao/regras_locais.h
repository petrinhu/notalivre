// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Etapa das regras locais da fachada da DPS (NL-3.M, DA-3M.1): o ValidadorLocal do Back aplicado à Dps montada, com o catálogo real.
// Header interno da Mid (como leitores_dps.h): a fachada pública é dps_validar.h.
#ifndef NL_MID_COMPOSICAO_REGRAS_LOCAIS_H
#define NL_MID_COMPOSICAO_REGRAS_LOCAIS_H

#include <nl/base/erro.h>
#include <nl/mid/composicao/dps_validar.h>

#include <vector>

namespace nl::back {
class CatalogoLocalidades;
class Dps;
}

namespace nl::mid::composicao {

/// Erros de todas as regras E e avisos do ValidadorLocal, sem parar na primeira violação (NL-1.5). Os avisos já estão no tipo da fachada.
struct ResultadoRegrasLocais
{
    std::vector<nl::base::Erro> erros;
    std::vector<AvisoDps> avisos;
};

/// Aplica o ValidadorLocal padrão à DPS. Não lê arquivo e não altera a Dps: o catálogo já está carregado (DA-1.25).
ResultadoRegrasLocais aplicarRegrasLocais(const nl::back::Dps& dps, const nl::back::CatalogoLocalidades& catalogo);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_REGRAS_LOCAIS_H
