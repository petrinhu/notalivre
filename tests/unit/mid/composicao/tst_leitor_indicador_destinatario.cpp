// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do leitor do indDest da raiz da entrada (NL-3.L, leiaute de 24/11/2026): "1" e "0" lidos como estão, ausente vira 0,
// "2" e tipo não texto recusados, e ibscbs.indDest (o lugar antigo) recusado pelo leitor do grupo IBS/CBS. JSON literal, sem I/O.
// Slots sem vírgula (L-45).

#include "leitores_dps.h"

#include <nl/back/dps/indicador_destinatario.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <expected>
#include <string>

namespace nb = nl::back;
namespace leit = nl::mid::composicao::leitores;

class TstLeitorIndicadorDestinatario : public QObject
{
    Q_OBJECT

private slots:
    void raizUmLeComoOutroDestinatario();
    void raizZeroLeComoTomadorEDestinatario();
    void raizAusenteViraZero();
    void raizDoisRecusadoComCodigoDoValor();
    void tipoNaoTextoRecusadoComTipoInvalido();
    void ibscbsIndDestRecusadoComCampoForaDoLugar();
};

namespace {

/// Objeto JSON literal, a raiz da entrada como o leitor a recebe.
QJsonObject objetoDe(const char* json)
{
    return QJsonDocument::fromJson(QByteArray{json}).object();
}

/// Código do erro, ou texto vazio quando a leitura deu certo.
template <typename T>
QString codigoDe(const std::expected<T, nl::base::Erro>& resultado)
{
    return resultado ? QString{} : QString::fromStdString(resultado.error().codigo());
}

} // namespace

void TstLeitorIndicadorDestinatario::raizUmLeComoOutroDestinatario()
{
    // Arrange: raiz com indDest "1" (destinatário é outra pessoa).
    const QJsonObject raiz = objetoDe(R"({"indDest": "1"})");

    // Act
    const auto indDest = leit::lerIndicadorDestinatario(raiz);

    // Assert
    QVERIFY2(indDest.has_value(), qPrintable(codigoDe(indDest)));
    QVERIFY(indDest->valor() == nb::IndicadorDestinatario::Valor::outroDestinatario);
}

void TstLeitorIndicadorDestinatario::raizZeroLeComoTomadorEDestinatario()
{
    // Arrange: raiz com indDest "0" (o próprio tomador). O padrão também é 0, então este slot só prova a leitura do texto
    // presente; quem prova que a raiz é lida de fato é raizUmLeComoOutroDestinatario.
    const QJsonObject raiz = objetoDe(R"({"indDest": "0"})");

    // Act
    const auto indDest = leit::lerIndicadorDestinatario(raiz);

    // Assert
    QVERIFY2(indDest.has_value(), qPrintable(codigoDe(indDest)));
    QVERIFY(indDest->valor() == nb::IndicadorDestinatario::Valor::tomadorEDestinatario);
}

void TstLeitorIndicadorDestinatario::raizAusenteViraZero()
{
    // Arrange: a raiz não traz a chave; o padrão é "0", o destinatário é o próprio tomador.
    const QJsonObject raiz = objetoDe(R"({"tpAmb": "2"})");

    // Act
    const auto indDest = leit::lerIndicadorDestinatario(raiz);

    // Assert
    QVERIFY2(indDest.has_value(), qPrintable(codigoDe(indDest)));
    QVERIFY(indDest->valor() == nb::IndicadorDestinatario::Valor::tomadorEDestinatario);
}

void TstLeitorIndicadorDestinatario::raizDoisRecusadoComCodigoDoValor()
{
    // Arrange: "2" está fora do conjunto do XSD (0 e 1).
    const QJsonObject raiz = objetoDe(R"({"indDest": "2"})");

    // Act
    const auto indDest = leit::lerIndicadorDestinatario(raiz);

    // Assert
    QVERIFY(!indDest.has_value());
    QCOMPARE(codigoDe(indDest), QStringLiteral("indicadordestinatario.valor"));
}

void TstLeitorIndicadorDestinatario::tipoNaoTextoRecusadoComTipoInvalido()
{
    // Arrange: número JSON 1 e booleano true. O XSD quer texto "1"; aceitar o número esconderia a regra do tipo.
    const QJsonObject numero = objetoDe(R"({"indDest": 1})");
    const QJsonObject booleano = objetoDe(R"({"indDest": true})");

    // Act
    const auto porNumero = leit::lerIndicadorDestinatario(numero);
    const auto porBooleano = leit::lerIndicadorDestinatario(booleano);

    // Assert
    QCOMPARE(codigoDe(porNumero), QStringLiteral("entrada.tipo_invalido"));
    QCOMPARE(codigoDe(porBooleano), QStringLiteral("entrada.tipo_invalido"));
}

void TstLeitorIndicadorDestinatario::ibscbsIndDestRecusadoComCampoForaDoLugar()
{
    // Arrange: grupo ibscbs completo e válido até o indDest (cIndOp e tpOper vêm antes na leitura), com o indDest no lugar antigo.
    const QJsonObject raiz = objetoDe(R"({"ibscbs": {"cIndOp": "000001", "tpOper": "1", "indDest": "0",
        "cst": "000", "cClassTrib": "000001"}})");

    // Act
    const auto ibscbs = leit::lerIbscbs(raiz);

    // Assert
    QVERIFY(!ibscbs.has_value());
    QCOMPARE(codigoDe(ibscbs), QStringLiteral("entrada.campo_fora_do_lugar"));
    QCOMPARE(QString::fromStdString(ibscbs.error().campo().value_or(std::string{})), QStringLiteral("ibscbs.indDest"));
}

QTEST_GUILESS_MAIN(TstLeitorIndicadorDestinatario)
#include "tst_leitor_indicador_destinatario.moc"
