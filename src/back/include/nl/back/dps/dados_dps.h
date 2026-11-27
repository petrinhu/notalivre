// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Dados já validados para montar a DPS (NL-1.4): entrada de Dps::montar, um campo por tipo validado. Não é entidade: quem valida é cada tipo.

#ifndef NL_BACK_DPS_DADOS_DPS_H
#define NL_BACK_DPS_DADOS_DPS_H

#include <optional>

#include <QDate>
#include <QDateTime>

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/ibscbs.h>
#include <nl/back/dps/indicador_destinatario.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/serie.h>
#include <nl/back/dps/servico.h>
#include <nl/back/dps/substituicao.h>
#include <nl/back/dps/tipo_ambiente.h>
#include <nl/back/dps/tomador.h>
#include <nl/back/dps/valores_servico.h>
#include <nl/back/dps/texto_limitado.h>
#include <nl/back/identificacao/codigo_municipio.h>


namespace nl::back {

/// Versão do aplicativo emissor, 1 a 20 caracteres (TSVerAplic, tiposSimples_v1.01.xsd:149-158).
using VerAplic = TextoLimitado<1, 20, Alfabeto::tsString>;

/// Campos de infDPS. tpEmit (sempre 1) e finNFSe (sempre 0) não entram aqui: são constantes de Dps (DA-1.3).
/// indDest fica na raiz, entre toma e serv, e é obrigatório em toda DPS desde o leiaute de 24/11/2026
/// (TCInfDPS, tiposComplexos_v1.01.xsd:845; NL-3.L). Antes ficava dentro do grupo IBSCBS.
struct DadosDps
{
    TipoAmbiente tpAmb;
    QDateTime dhEmi;
    VerAplic verAplic;
    Serie serie;
    NumeroDps nDps;
    QDate dCompet;
    CodigoMunicipio cLocEmi;
    std::optional<Substituicao> subst;
    Prestador prest;
    std::optional<Tomador> toma;
    IndicadorDestinatario indDest;
    Servico serv;
    ValoresServico valores;
    std::optional<Ibscbs> ibscbs;
};

} // namespace nl::back

#endif // NL_BACK_DPS_DADOS_DPS_H
