// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Grupos de serviço e de valores do TCDPS: serv, valores, trib (tribMun e totTrib) (NL-1.6b).
#include "codigos.h"
#include "grupos.h"

#include <nl/back/dps/total_tributos.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>
#include <nl/back/dps/tributos_monetarios.h>
#include <nl/back/dps/tributos_percentuais.h>
#include <nl/back/dps/valores_servico.h>

#include <variant>

namespace nl::mid::sefin::prodrest_20261020::grupos {

namespace nb = nl::back;

namespace {

/// tribMun (TCTribMunicipal): tribISSQN, tpRetISSQN e, se houver, pAliq. As outras alternativas do XSD não entram na v1.
void escreveTribMunicipal(QXmlStreamWriter& w, const nb::TributacaoMunicipal& municipal)
{
    w.writeStartElement(QStringLiteral("tribMun"));
    w.writeTextElement(QStringLiteral("tribISSQN"), codigos::tribIssqn(municipal.tribISSQN().valor()));
    w.writeTextElement(QStringLiteral("tpRetISSQN"), codigos::retencaoIssqn(municipal.tpRetISSQN().valor()));
    if (municipal.pAliq()) {
        w.writeTextElement(QStringLiteral("pAliq"), municipal.pAliq()->texto());
    }
    w.writeEndElement();
}

/// totTrib (TCTribTotal): uma das quatro alternativas da escolha do XSD, conforme o tipo que a Dps carrega.
void escreveTotTrib(QXmlStreamWriter& w, const nb::TotalTributos& total)
{
    w.writeStartElement(QStringLiteral("totTrib"));
    std::visit(Visitante{
                   [&w](const nb::TributosMonetarios& monetarios) {
                       w.writeStartElement(QStringLiteral("vTotTrib"));
                       w.writeTextElement(QStringLiteral("vTotTribFed"), monetarios.vTotTribFed().paraLeiaute());
                       w.writeTextElement(QStringLiteral("vTotTribEst"), monetarios.vTotTribEst().paraLeiaute());
                       w.writeTextElement(QStringLiteral("vTotTribMun"), monetarios.vTotTribMun().paraLeiaute());
                       w.writeEndElement();
                   },
                   [&w](const nb::TributosPercentuais& percentuais) {
                       w.writeStartElement(QStringLiteral("pTotTrib"));
                       w.writeTextElement(QStringLiteral("pTotTribFed"), percentuais.pTotTribFed().texto());
                       w.writeTextElement(QStringLiteral("pTotTribEst"), percentuais.pTotTribEst().texto());
                       w.writeTextElement(QStringLiteral("pTotTribMun"), percentuais.pTotTribMun().texto());
                       w.writeEndElement();
                   },
                   [&w](const nb::IndicadorTotalTributos& indicador) {
                       w.writeTextElement(QStringLiteral("indTotTrib"), codigos::indTotTrib(indicador.valor()));
                   },
                   [&w](const nb::PercentualTributoSn& percentual) {
                       w.writeTextElement(QStringLiteral("pTotTribSN"), percentual.texto());
                   },
               },
               total);
    w.writeEndElement();
}

/// trib (TCInfoTributacao) de valores: tribMun e totTrib. tribFed não entra na v1 (fora do escopo do Anexo da Onda 1).
void escreveTributacao(QXmlStreamWriter& w, const nb::Tributacao& tributacao)
{
    w.writeStartElement(QStringLiteral("trib"));
    escreveTribMunicipal(w, tributacao.tribMun());
    escreveTotTrib(w, tributacao.totTrib());
    w.writeEndElement();
}

} // namespace

void escreveServico(QXmlStreamWriter& w, const nb::Servico& servico)
{
    w.writeStartElement(QStringLiteral("serv"));
    w.writeStartElement(QStringLiteral("locPrest"));
    w.writeTextElement(QStringLiteral("cLocPrestacao"), servico.cLocPrestacao().texto());
    w.writeEndElement();
    w.writeStartElement(QStringLiteral("cServ"));
    w.writeTextElement(QStringLiteral("cTribNac"), servico.cTribNac().texto());
    if (servico.cNBS()) {
        w.writeTextElement(QStringLiteral("cNBS"), servico.cNBS()->texto());
    }
    w.writeTextElement(QStringLiteral("xDescServ"), servico.xDescServ().texto());
    w.writeEndElement();
    w.writeEndElement();
}

void escreveValores(QXmlStreamWriter& w, const nb::ValoresServico& valores)
{
    w.writeStartElement(QStringLiteral("valores"));
    w.writeStartElement(QStringLiteral("vServPrest"));
    w.writeTextElement(QStringLiteral("vServ"), valores.vServ().paraLeiaute());
    w.writeEndElement();
    escreveTributacao(w, valores.trib());
    w.writeEndElement();
}

} // namespace nl::mid::sefin::prodrest_20261020::grupos
