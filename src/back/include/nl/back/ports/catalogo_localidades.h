// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Port CatalogoLocalidades (NL-1.5, L-33): existência de município e de país na TAB.LOC.GERAL e nas tabelas de país do Anexo A.
//
// CONTRATO (DA-1.25): o catálogo já está carregado e completo quando o ValidadorLocal é montado. As consultas são em
// memória, sem I/O e sem erro de infraestrutura: uma falha de carga é erro da FÁBRICA do adaptador (NL-1.10, Mid), que
// devolve std::expected antes de montar o validador. Se a NL-5.3 trouxer consulta remota sob demanda, o port passa a
// devolver expected<bool, Erro> (DA-1.25, opção f2).
//
// O código "0000000" (Águas Marítimas) é uma linha da TAB.LOC.GERAL e é consultado como os demais (DA-1.10 (a)).
#ifndef NL_BACK_PORTS_CATALOGO_LOCALIDADES_H
#define NL_BACK_PORTS_CATALOGO_LOCALIDADES_H

#include <nl/back/dps/codigo_pais.h>
#include <nl/back/identificacao/codigo_municipio.h>

namespace nl::back {

class CatalogoLocalidades
{
public:
    virtual ~CatalogoLocalidades();

    /// Município da TAB.LOC.GERAL: código do IBGE do Anexo A e a linha 0000000 (Águas Marítimas).
    virtual bool municipioExiste(const CodigoMunicipio& codigo) const = 0;

    /// País da tabela ISO2 do Anexo A (cPais). A regra E0246 recusa BR e ZZ antes de consultar.
    virtual bool paisExiste(const CodigoPais& codigo) const = 0;
};

} // namespace nl::back

#endif // NL_BACK_PORTS_CATALOGO_LOCALIDADES_H
