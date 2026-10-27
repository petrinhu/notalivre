// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do validador XSD da Foundation (NL-1.6a). Fixtures escritas à mão em tests/fixtures/xsd, régua do xmllint
// no oráculo CTest (oraculo_xmllint.sh). Os XSD vêm do build (third_party/nfse-xsd), nunca da árvore.

#include <nl/foundation/xml/erros_xsd.h>
#include <nl/foundation/xml/esquema.h>
#include <nl/foundation/xml/validador_xsd.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include <expected>
#include <string>

namespace {

const QString kDiretorioXsd = QStringLiteral(NL_XSD_PRODREST_DIR);
const QString kManifesto = QStringLiteral(NL_XSD_PRODREST_MANIFESTO);
const QString kRaiz = QStringLiteral(NL_XSD_PRODREST_RAIZ);
const QString kFixtures = QStringLiteral(NL_FIXTURES_XSD);

using nl::foundation::xml::ErrosXsd;
using nl::foundation::xml::Esquema;
using nl::foundation::xml::ValidadorXsd;

// Lê uma fixture; arquivo ilegível vira conteúdo vazio, que as asserções dos testes reprovam.
QByteArray lerFixture(const QString& nome)
{
    QFile arquivo{QDir{kFixtures}.filePath(nome)};
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Mensagens de todos os erros concatenadas, para asserções de conteúdo.
QString mensagensDe(const ErrosXsd& erros)
{
    QStringList partes;
    for (const auto& erro : erros.erros()) {
        partes << QString::fromStdString(erro.mensagem());
    }
    return partes.join(QStringLiteral(" | "));
}

// Descrição para QVERIFY2: o macro avalia a descrição nos dois ramos, então só lê o erro quando houve falha.
template <typename T>
QString descricaoDe(const std::expected<T, ErrosXsd>& resultado)
{
    return resultado.has_value() ? QString{} : mensagensDe(resultado.error());
}

} // namespace

class TstValidadorXsd : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void dpsSemAssinaturaValidaNoXsd();
    void campoObrigatorioAusenteReprova();
    void xmlMalformadoReprova();
    void idForaDoPatternReprova();
    void shaErradoDoEsquemaRecusado();
    void fixturesConcordamComORegraDoNome();
};

void TstValidadorXsd::initTestCase()
{
    QVERIFY2(QFileInfo::exists(kManifesto), qPrintable(kManifesto));
    QVERIFY2(QFileInfo::exists(QDir{kDiretorioXsd}.filePath(kRaiz)), qPrintable(kRaiz));
}

void TstValidadorXsd::dpsSemAssinaturaValidaNoXsd()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("valida_dps_minima.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY2(resultado.has_value(), qPrintable(descricaoDe(resultado)));
}

void TstValidadorXsd::campoObrigatorioAusenteReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_campo_obrigatorio_ausente.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::invalido);
    // A mensagem precisa apontar o campo que falta (dhEmi), não só dizer "inválido".
    QVERIFY2(mensagensDe(resultado.error()).contains(QStringLiteral("dhEmi")), qPrintable(mensagensDe(resultado.error())));
    // Formato "not expected ... Expected is ( dhEmi )": o campo é o esperado (o que falta); o elemento que apareceu
    // no lugar (verAplic) fica na mensagem. Decisão documentada em libxml_util.h (campoDaMensagem).
    QVERIFY(resultado.error().erros().front().campo().has_value());
    QCOMPARE(QString::fromStdString(resultado.error().erros().front().campo().value()), QStringLiteral("dhEmi"));
    QVERIFY2(mensagensDe(resultado.error()).contains(QStringLiteral("verAplic")), qPrintable(mensagensDe(resultado.error())));
}

void TstValidadorXsd::xmlMalformadoReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_xml_malformado.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::xmlMalformado);
    QVERIFY(!resultado.error().erros().empty());
}

void TstValidadorXsd::idForaDoPatternReprova()
{
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QByteArray xml = lerFixture(QStringLiteral("invalida_id_fora_do_pattern.xml"));

    const auto resultado = ValidadorXsd::validar(xml, esquema.value());

    QVERIFY(!resultado.has_value());
    QCOMPARE(resultado.error().motivo(), ErrosXsd::Motivo::invalido);
    const QString msg = mensagensDe(resultado.error());
    QVERIFY2(msg.contains(QStringLiteral("pattern")), qPrintable(msg));
    QVERIFY2(msg.contains(QStringLiteral("Id")), qPrintable(msg));
    // Formato "Element '{ns}infDPS', attribute 'Id': [facet 'pattern']...": sem "Expected is", o campo é o elemento citado.
    QVERIFY(resultado.error().erros().front().campo().has_value());
    QCOMPARE(QString::fromStdString(resultado.error().erros().front().campo().value()), QStringLiteral("infDPS"));
}

void TstValidadorXsd::shaErradoDoEsquemaRecusado()
{
    // Cópia da pasta do XSD com um byte alterado num arquivo da cadeia (tiposSimples, incluído pelo DPS).
    QTemporaryDir temporario;
    QVERIFY(temporario.isValid());
    const QDir origem{kDiretorioXsd};
    for (const QString& nome : origem.entryList(QDir::Files)) {
        QVERIFY(QFile::copy(origem.filePath(nome), QDir{temporario.path()}.filePath(nome)));
    }
    QFile adulterado{QDir{temporario.path()}.filePath(QStringLiteral("tiposSimples_v1.01.xsd"))};
    QVERIFY(adulterado.open(QIODevice::Append));
    QCOMPARE(adulterado.write(" "), qint64{1});
    adulterado.close();

    const auto esquema = Esquema::abrir(temporario.path(), kManifesto, kRaiz);

    QVERIFY(!esquema.has_value());
    QCOMPARE(esquema.error().motivo(), ErrosXsd::Motivo::esquemaRecusado);
    QVERIFY2(mensagensDe(esquema.error()).contains(QStringLiteral("tiposSimples_v1.01.xsd")),
             qPrintable(mensagensDe(esquema.error())));
}

void TstValidadorXsd::fixturesConcordamComORegraDoNome()
{
    // Mesma régua do oráculo xmllint: valida_* deve validar, invalida_* deve falhar. Zero fixtures reprova.
    const auto esquema = Esquema::abrir(kDiretorioXsd, kManifesto, kRaiz);
    QVERIFY2(esquema.has_value(), qPrintable(descricaoDe(esquema)));
    const QStringList nomes = QDir{kFixtures}.entryList({QStringLiteral("*.xml")}, QDir::Files);
    QVERIFY2(!nomes.isEmpty(), "varredura vazia: nenhuma fixture em tests/fixtures/xsd");

    int analisados = 0;
    for (const QString& nome : nomes) {
        const bool deveValidar = nome.startsWith(QStringLiteral("valida_"));
        const bool deveFalhar = nome.startsWith(QStringLiteral("invalida_"));
        QVERIFY2(deveValidar || deveFalhar, qPrintable(QStringLiteral("nome sem veredito: %1").arg(nome)));

        const auto resultado = ValidadorXsd::validar(lerFixture(nome), esquema.value());
        QCOMPARE(resultado.has_value(), deveValidar);
        ++analisados;
    }
    QCOMPARE(analisados, static_cast<int>(nomes.size()));
}

QTEST_GUILESS_MAIN(TstValidadorXsd)
#include "tst_validador_xsd.moc"
