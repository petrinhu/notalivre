// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ValidadorLocal (NL-1.5): aplica todas as regras E e todos os avisos a uma Dps e devolve a lista inteira.
#ifndef NL_BACK_REGRAS_VALIDADOR_LOCAL_H
#define NL_BACK_REGRAS_VALIDADOR_LOCAL_H

#include <memory>
#include <vector>

#include <nl/back/dps/dps.h>
#include <nl/back/ports/catalogo_localidades.h>
#include <nl/back/regras/regra_aviso.h>
#include <nl/back/regras/regra_local.h>
#include <nl/back/regras/resultado_validacao.h>

namespace nl::back {

class ValidadorLocal
{
public:
    using Regras = std::vector<std::unique_ptr<RegraLocal>>;
    using Avisos = std::vector<std::unique_ptr<RegraAviso>>;

    ValidadorLocal(Regras regras, Avisos avisos);

    /// Conjunto da v1: as regras E que entram em NL-1.5 e o aviso D1. O catálogo é guardado por referência:
    /// quem chama mantém o catálogo vivo enquanto usa o validador.
    static ValidadorLocal padrao(const CatalogoLocalidades& catalogo);

    /// Não para na primeira violação: devolve todos os erros e todos os avisos.
    ResultadoValidacao validar(const Dps& dps) const;

private:
    Regras regras_;
    Avisos avisos_;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_VALIDADOR_LOCAL_H
