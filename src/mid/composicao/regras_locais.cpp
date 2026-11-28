// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Etapa das regras locais da fachada da DPS (NL-3.M, DA-3M.1). Ver regras_locais.h.
#include "regras_locais.h"

#include <nl/back/regras/aviso.h>
#include <nl/back/regras/resultado_validacao.h>
#include <nl/back/regras/validador_local.h>

namespace nl::mid::composicao {

namespace {

AvisoDps paraAvisoDps(const nl::back::Aviso& aviso)
{
    return AvisoDps{aviso.codigo(), aviso.campo(), aviso.mensagem()};
}

} // namespace

ResultadoRegrasLocais aplicarRegrasLocais(const nl::back::Dps& dps, const nl::back::CatalogoLocalidades& catalogo)
{
    const nl::back::ResultadoValidacao resultado = nl::back::ValidadorLocal::padrao(catalogo).validar(dps);
    ResultadoRegrasLocais saida;
    saida.erros = resultado.erros;
    saida.avisos.reserve(resultado.avisos.size());
    for (const nl::back::Aviso& aviso : resultado.avisos) {
        saida.avisos.push_back(paraAvisoDps(aviso));
    }
    return saida;
}

} // namespace nl::mid::composicao
