// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo falso em memória para os testes de NL-1.5 (L-33): o Back não lê tabela, o teste injeta o port.
// Espelha a TAB.LOC.GERAL: "0000000" (Águas Marítimas) é uma linha do catálogo, consultada como qualquer município (DA-1.10).
#ifndef NL_TESTE_BACK_REGRAS_CATALOGO_FALSO_H
#define NL_TESTE_BACK_REGRAS_CATALOGO_FALSO_H

#include <nl/back/ports/catalogo_localidades.h>

namespace teste {

/// Município 4305108 e país US existem. "0000000" existe por padrão; com aguasMaritimas = false ele fica fora do catálogo.
class CatalogoFalso final : public nl::back::CatalogoLocalidades
{
public:
    explicit CatalogoFalso(bool aguasMaritimas = true) : aguasMaritimas_{aguasMaritimas} {}

    bool municipioExiste(const nl::back::CodigoMunicipio& codigo) const override
    {
        if (codigo.texto() == QStringLiteral("0000000")) {
            return aguasMaritimas_;
        }
        return codigo.texto() == QStringLiteral("4305108");
    }

    bool paisExiste(const nl::back::CodigoPais& codigo) const override
    {
        return codigo.texto() == QStringLiteral("US");
    }

private:
    bool aguasMaritimas_;
};

} // namespace teste

#endif // NL_TESTE_BACK_REGRAS_CATALOGO_FALSO_H
