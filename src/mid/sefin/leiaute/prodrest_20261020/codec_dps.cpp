// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Codec da DPS (NL-1.6b): raiz DPS e grupo infDPS. Os grupos de pessoa, serviço e IBS/CBS ficam nos arquivos grupos_*.
#include <nl/mid/sefin/leiaute/prodrest_20261020/codec_dps.h>

#include "codigos.h"
#include "grupos.h"
#include "limites_leiaute.h"

#include <nl/back/dps/dps.h>
#include <nl/back/dps/substituicao.h>

#include <QDate>
#include <QDateTime>
#include <QXmlStreamWriter>

#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

namespace nb = nl::back;

namespace {

constexpr auto kNamespaceNfse = "http://www.sped.fazenda.gov.br/nfse";

QString comoQString(std::string_view texto)
{
    return QString::fromUtf8(texto.data(), static_cast<qsizetype>(texto.size()));
}

/// serie (TSSerieDPS): sem zeros à esquerda. O texto do Back tem as cinco posições do Id.
QString serieNoXml(const nb::Serie& serie)
{
    return QString::number(serie.texto().toInt());
}

/// substituicao (TCSubstituicao): chave substituída, código da justificativa e motivo opcional.
void escreveSubstituicao(QXmlStreamWriter& w, const nb::Substituicao& substituicao)
{
    w.writeStartElement(QStringLiteral("subst"));
    w.writeTextElement(QStringLiteral("chSubstda"), substituicao.chSubstda().texto());
    w.writeTextElement(QStringLiteral("cMotivo"), codigos::justificativaSubstituicao(substituicao.cMotivo().valor()));
    if (substituicao.xMotivo()) {
        w.writeTextElement(QStringLiteral("xMotivo"), substituicao.xMotivo()->texto());
    }
    w.writeEndElement();
}

/// infDPS (TCInfDPS), na ordem da xs:sequence do XSD. tpEmit e finNFSe vêm das constantes da Dps (DA-1.3).
void escreveInfDps(QXmlStreamWriter& w, const nb::Dps& dps)
{
    const nb::DadosDps& dados = dps.dados();
    w.writeStartElement(QStringLiteral("infDPS"));
    w.writeAttribute(QStringLiteral("Id"), dps.id().texto());
    w.writeTextElement(QStringLiteral("tpAmb"), codigos::ambiente(dados.tpAmb.valor()));
    w.writeTextElement(QStringLiteral("dhEmi"), dps.dhEmi().texto());
    w.writeTextElement(QStringLiteral("verAplic"), dados.verAplic.texto());
    w.writeTextElement(QStringLiteral("serie"), serieNoXml(dados.serie));
    w.writeTextElement(QStringLiteral("nDPS"), QString::number(dados.nDps.valor()));
    w.writeTextElement(QStringLiteral("dCompet"), dados.dCompet.toString(Qt::ISODate));
    w.writeTextElement(QStringLiteral("tpEmit"), QString::number(nb::Dps::tpEmit()));
    w.writeTextElement(QStringLiteral("finNFSe"), QString::number(nb::Dps::finNFSe()));
    w.writeTextElement(QStringLiteral("cLocEmi"), dados.cLocEmi.texto());
    if (dados.subst) {
        escreveSubstituicao(w, *dados.subst);
    }
    grupos::escrevePrestador(w, dados.prest);
    if (dados.toma) {
        grupos::escreveTomador(w, *dados.toma);
    }
    grupos::escreveServico(w, dados.serv);
    grupos::escreveValores(w, dados.valores);
    if (dados.ibscbs) {
        grupos::escreveIbscbs(w, *dados.ibscbs);
    }
    w.writeEndElement();
}

} // namespace

QByteArray CodecDps::serializar(const nb::Dps& dps)
{
    QByteArray xml;
    QXmlStreamWriter w{&xml};
    w.setAutoFormatting(true);
    w.writeStartDocument(QStringLiteral("1.0"));
    w.writeStartElement(QStringLiteral("DPS"));
    w.writeDefaultNamespace(QString::fromLatin1(kNamespaceNfse));
    w.writeAttribute(QStringLiteral("versao"), comoQString(kVersaoLeiaute));
    escreveInfDps(w, dps);
    w.writeEndElement();
    w.writeEndDocument();
    return xml;
}

} // namespace nl::mid::sefin::prodrest_20261020
