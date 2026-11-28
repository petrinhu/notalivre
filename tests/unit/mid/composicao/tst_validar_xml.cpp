// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da revalidação do XML no XSD depois de assinar (NL-2.9, DA-2.51) e da ordem das etapas da fachada (NL-3.M, DA-3M.2).
// O controle positivo é a DPS de entrada válida; o negativo é um XML que não confere no leiaute. Só fixtures sintéticas
// (tests/e2e/dps_validar/entradas) e as tabelas do Anexo A do build.
#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTemporaryDir>
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

nl::mid::composicao::LocalizacaoTabelas tabelasDeTeste()
{
    return nl::mid::composicao::LocalizacaoTabelas{QString::fromUtf8(NL_TABELAS_DIR)};
}

// Entrada MEI com indDest 1 na raiz: a regra E0910 (local) reprova a DPS sem o XSD ser consultado.
QByteArray entradaComIndDest1()
{
    QJsonObject raiz = QJsonDocument::fromJson(lerEntrada(NL_ENTRADA_MEI)).object();
    raiz.insert(QStringLiteral("indDest"), QStringLiteral("1"));
    return QJsonDocument(raiz).toJson(QJsonDocument::Compact);
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
    // NL-2.10b, BAIXA-4: manifesto do XSD com hash diferente do pin embutido é recusado antes de abrir o esquema.
    void validarXmlRecusaManifestoDivergente();
    // NL-3.M (DA-3M.4 e DA-3M.8): diretório de tabelas ausente é ambiente (natureza ambiente, código tabelas.ausente), sem XML.
    void executarTabelasAusentesSaemComAmbiente();
    // NL-3.M (DA-3M.2): a etapa das regras locais vem antes do XSD. Com indDest 1 e manifesto divergente, o erro é E0910, não o do manifesto.
    void executarRegraLocalReprovaAntesDoXsd();
};

void TstValidarXml::validarXmlAceitaDpsValida()
{
    const QByteArray json = lerEntrada(NL_ENTRADA_MEI);
    QVERIFY2(!json.isEmpty(), "a entrada de teste tem de existir");
    const auto dps = nl::mid::composicao::DpsValidar::executar(json, esquemaDeTeste(), tabelasDeTeste());
    QVERIFY2(dps.has_value(), "a DPS de teste tem de passar na entrada");
    const auto revalidado = nl::mid::composicao::DpsValidar::validarXml(dps->xml, esquemaDeTeste());
    QVERIFY2(revalidado.has_value(), "o XML válido tem de passar na revalidação");
}

void TstValidarXml::validarXmlRecusaXmlQueNaoConfereNoXsd()
{
    const QByteArray xml = QByteArrayLiteral("<DPS xmlns=\"http://www.sped.fazenda.gov.br/nfse\"/>");
    const auto recusado = nl::mid::composicao::DpsValidar::validarXml(xml, esquemaDeTeste());
    QVERIFY2(!recusado.has_value(), "XML que não confere no XSD tem de ser recusado");
    QVERIFY2(recusado.error().natureza == nl::mid::composicao::NaturezaFalha::reprovada, "a recusa é do documento, não do ambiente");
}

void TstValidarXml::validarXmlRecusaManifestoDivergente()
{
    // Cópia do SHA256SUMS com uma linha a mais: o hash do manifesto deixa de ser o embutido no build (pin, BAIXA-4).
    QFile origem(QString::fromUtf8(NL_XSD_PRODREST_MANIFESTO));
    QVERIFY2(origem.open(QIODevice::ReadOnly), "o SHA256SUMS de origem tem de existir");
    const QByteArray conteudo = origem.readAll();
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString caminho = pasta.filePath(QStringLiteral("SHA256SUMS"));
    QFile copia(caminho);
    QVERIFY(copia.open(QIODevice::WriteOnly));
    copia.write(conteudo);
    copia.write("# linha acrescentada: o hash deste manifesto não é o embutido\n");
    copia.close();
    const QByteArray json = lerEntrada(NL_ENTRADA_MEI);
    const auto dps = nl::mid::composicao::DpsValidar::executar(
        json, nl::mid::composicao::LocalizacaoEsquemaDps{QString::fromUtf8(NL_XSD_PRODREST_DIR), caminho}, tabelasDeTeste());
    QVERIFY2(!dps.has_value(), "manifesto fora do pin embutido tem de ser recusado");
    QCOMPARE(QString::fromStdString(dps.error().erros.front().codigo()), QStringLiteral("configuracao.xsd_manifesto_nao_fixado"));
}

void TstValidarXml::executarTabelasAusentesSaemComAmbiente()
{
    const QByteArray json = lerEntrada(NL_ENTRADA_MEI);
    QVERIFY2(!json.isEmpty(), "a entrada de teste tem de existir");
    const auto dps = nl::mid::composicao::DpsValidar::executar(
        json, esquemaDeTeste(), nl::mid::composicao::LocalizacaoTabelas{QStringLiteral("/caminho/que/nao/existe/tabelas")});
    QVERIFY2(!dps.has_value(), "tabelas ausentes têm de recusar a validação");
    QVERIFY2(dps.error().natureza == nl::mid::composicao::NaturezaFalha::ambiente, "tabela ausente é do ambiente, não da DPS");
    QCOMPARE(QString::fromStdString(dps.error().erros.front().codigo()), QStringLiteral("tabelas.ausente"));
}

void TstValidarXml::executarRegraLocalReprovaAntesDoXsd()
{
    // Manifesto do XSD com uma linha a mais: se o XSD fosse consultado antes, o erro seria o do pin, não o da regra.
    QFile origem(QString::fromUtf8(NL_XSD_PRODREST_MANIFESTO));
    QVERIFY2(origem.open(QIODevice::ReadOnly), "o SHA256SUMS de origem tem de existir");
    const QByteArray conteudo = origem.readAll();
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString caminho = pasta.filePath(QStringLiteral("SHA256SUMS"));
    QFile copia(caminho);
    QVERIFY(copia.open(QIODevice::WriteOnly));
    copia.write(conteudo);
    copia.write("# linha acrescentada: o hash deste manifesto não é o embutido\n");
    copia.close();
    const QByteArray json = entradaComIndDest1();
    const auto dps = nl::mid::composicao::DpsValidar::executar(
        json, nl::mid::composicao::LocalizacaoEsquemaDps{QString::fromUtf8(NL_XSD_PRODREST_DIR), caminho}, tabelasDeTeste());
    QVERIFY2(!dps.has_value(), "DPS com regra local reprovada tem de ser recusada");
    QVERIFY2(dps.error().natureza == nl::mid::composicao::NaturezaFalha::reprovada, "regra local é culpa do documento");
    QCOMPARE(QString::fromStdString(dps.error().erros.front().codigo()), QStringLiteral("E0910"));
}

QTEST_APPLESS_MAIN(TstValidarXml)
#include "tst_validar_xml.moc"
