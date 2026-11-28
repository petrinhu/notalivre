// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Consultas do catálogo de localidades (NL-3.M, DA-3M.4). O conjunto já está pronto quando o objeto existe: a carga é em
// carregar_catalogo.cpp, e aqui só há consulta em memória, sem I/O (contrato DA-1.25).
#include <nl/mid/tabelas/catalogo_localidades_embutido.h>

#include <nl/back/dps/codigo_pais.h>
#include <nl/back/identificacao/codigo_municipio.h>

#include <utility>

namespace nl::mid::tabelas {

CatalogoLocalidadesEmbutido::CatalogoLocalidadesEmbutido(QSet<QString> municipios, QSet<QString> paises)
    : municipios_{std::move(municipios)}, paises_{std::move(paises)}
{
}

bool CatalogoLocalidadesEmbutido::municipioExiste(const nl::back::CodigoMunicipio& codigo) const
{
    return municipios_.contains(codigo.texto());
}

bool CatalogoLocalidadesEmbutido::paisExiste(const nl::back::CodigoPais& codigo) const
{
    return paises_.contains(codigo.texto());
}

} // namespace nl::mid::tabelas
