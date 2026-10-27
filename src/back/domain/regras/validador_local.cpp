// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ValidadorLocal (NL-1.5): aplica todas as regras E e todos os avisos sem parar na primeira violação.

#include <nl/back/regras/validador_local.h>

#include <utility>

#include <nl/back/regras/aviso_d1.h>
#include <nl/back/regras/regra_e0202.h>
#include <nl/back/regras/regra_e0204.h>
#include <nl/back/regras/regra_e0237.h>
#include <nl/back/regras/regra_e0241.h>
#include <nl/back/regras/regra_e0246.h>
#include <nl/back/regras/regra_e0302.h>
#include <nl/back/regras/regra_e0390.h>
#include <nl/back/regras/regra_e0532.h>
#include <nl/back/regras/regra_e0583.h>
#include <nl/back/regras/regra_e0710.h>
#include <nl/back/regras/regra_e0850.h>
#include <nl/back/regras/regra_e0903.h>

namespace nl::back {

ValidadorLocal::ValidadorLocal(Regras regras, Avisos avisos) : regras_{std::move(regras)}, avisos_{std::move(avisos)} {}

ValidadorLocal ValidadorLocal::padrao(const CatalogoLocalidades& catalogo)
{
    Regras regras;
    regras.push_back(std::make_unique<RegraE0202>());
    regras.push_back(std::make_unique<RegraE0204>());
    regras.push_back(std::make_unique<RegraE0237>());
    regras.push_back(std::make_unique<RegraE0241>());
    regras.push_back(std::make_unique<RegraE0246>(catalogo));
    regras.push_back(std::make_unique<RegraE0302>(catalogo));
    regras.push_back(std::make_unique<RegraE0390>());
    regras.push_back(std::make_unique<RegraE0532>());
    regras.push_back(std::make_unique<RegraE0583>());
    regras.push_back(std::make_unique<RegraE0710>());
    regras.push_back(std::make_unique<RegraE0850>());
    regras.push_back(std::make_unique<RegraE0903>());

    Avisos avisos;
    avisos.push_back(std::make_unique<AvisoD1>());
    return ValidadorLocal{std::move(regras), std::move(avisos)};
}

ResultadoValidacao ValidadorLocal::validar(const Dps& dps) const
{
    ResultadoValidacao resultado;
    for (const auto& regra : regras_) {
        if (auto erro = regra->verificar(dps)) {
            resultado.erros.push_back(std::move(*erro));
        }
    }
    for (const auto& aviso : avisos_) {
        if (auto achado = aviso->avaliar(dps)) {
            resultado.avisos.push_back(std::move(*achado));
        }
    }
    return resultado;
}

} // namespace nl::back
