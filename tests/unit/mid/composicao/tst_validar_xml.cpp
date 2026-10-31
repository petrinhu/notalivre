// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da revalidação do XML no XSD depois de assinar (NL-2.9, DA-2.51). O controle positivo é a DPS de entrada válida; o
// negativo é um XML que não confere no leiaute. Só fixtures sintéticas (tests/e2e/dps_validar/entradas).
#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QtTest>

namespace {

QByteArray lerEntrada(const char* caminho)
{
    QFile arquivo(QString::fromUtf8(caminho));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

nl::mid::composicao::LocalizacaoEsquemaDps esquemaDeTeste()
{
    return nl::mid::composicao::LocalizacaoEsquemaDps{QString::fromUtf8(NL_XSD_PRODREST_DIR), QString::fromUtf8(NL_XSD_PRODREST_MANIFESTO)};
}

} // namespace

class TstValidarXml : public QObject
{
    Q_OBJECT

private slots:
    // Controle positivo: o XML que a validação de entrada produz passa na revalidação.
    void validarXmlAceitaDpsValida();
    // Um XML que não confere no leiaute é reprovado, com a natureza de documento (não de ambiente).
    void validarXmlRecusaXmlQueNaoConfereNoXsd();
};

void TstValidarXml::validarXmlAceitaDpsValida()
{
    const QByteArray json = lerEntrada(NL_ENTRADA_MEI);
    QVERIFY2(!json.isEmpty(), "a entrada de teste tem de existir");
    const auto xml = nl::mid::composicao::DpsValidar::executar(json, esquemaDeTeste());
    QVERIFY2(xml.has_value(), "a DPS de teste tem de passar na entrada");
    const auto revalidado = nl::mid::composicao::DpsValidar::validarXml(*xml, esquemaDeTeste());
    QVERIFY2(revalidado.has_value(), "o XML válido tem de passar na revalidação");
}

void TstValidarXml::validarXmlRecusaXmlQueNaoConfereNoXsd()
{
    const QByteArray xml = QByteArrayLiteral("<DPS xmlns=\"http://www.sped.fazenda.gov.br/nfse\"/>");
    const auto recusado = nl::mid::composicao::DpsValidar::validarXml(xml, esquemaDeTeste());
    QVERIFY2(!recusado.has_value(), "XML que não confere no XSD tem de ser recusado");
    QVERIFY2(recusado.error().natureza == nl::mid::composicao::NaturezaFalha::reprovada, "a recusa é do documento, não do ambiente");
}

QTEST_APPLESS_MAIN(TstValidarXml)
#include "tst_validar_xml.moc"
