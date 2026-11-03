// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do leitor de inscrição da CLI (NL-2.10h, H4): CNPJ e CPF com zeros à esquerda chegam como texto JSON e não podem virar número.
// Slots sem vírgula (L-45). Fixtures fictícias com DV pelo módulo 11, calculados no contêiner.

#include "leitores_comuns.h"

#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

#include <QJsonObject>
#include <QJsonValue>
#include <QtTest>

#include <string>
#include <variant>

namespace leit = nl::mid::composicao::leitores;
namespace nb = nl::back;

class TstLeitorInscricao : public QObject
{
    Q_OBJECT

private slots:
    void cnpjTextoComZeroInicialPreservaZeros();
    void cnpjTextoComDoisZerosIniciaisPreservaZeros();
    void cpfTextoComZeroInicialPreservaZero();
    void cnpjNumericoJsonRecusado();
    void cpfNumericoJsonRecusado();
    void cnpjECpfJuntosReprovam();
    void semCnpjNemCpfReprova();
};

void TstLeitorInscricao::cnpjTextoComZeroInicialPreservaZeros()
{
    // Fixture 01234567890107: o texto JSON tem de chegar ao Cnpj com o zero da frente.
    const QJsonObject pai{{"cnpj", QStringLiteral("01234567890107")}};
    const auto inscricao = leit::lerInscricao(pai, "emitente");
    QVERIFY2(inscricao.has_value(), "CNPJ em texto com zero à frente foi recusado");
    const auto* cnpj = std::get_if<nb::Cnpj>(&*inscricao);
    QVERIFY(cnpj != nullptr);
    QCOMPARE(cnpj->texto(), QStringLiteral("01234567890107"));
}

void TstLeitorInscricao::cnpjTextoComDoisZerosIniciaisPreservaZeros()
{
    // Fixture 00012345678978: dois zeros à frente, nenhum pode sumir na leitura.
    const QJsonObject pai{{"cnpj", QStringLiteral("00012345678978")}};
    const auto inscricao = leit::lerInscricao(pai, "emitente");
    QVERIFY2(inscricao.has_value(), "CNPJ em texto com 00 à frente foi recusado");
    const auto* cnpj = std::get_if<nb::Cnpj>(&*inscricao);
    QVERIFY(cnpj != nullptr);
    QCOMPARE(cnpj->texto(), QStringLiteral("00012345678978"));
}

void TstLeitorInscricao::cpfTextoComZeroInicialPreservaZero()
{
    // Fixture 01234567890: o CPF em texto tem de chegar com 11 dígitos.
    const QJsonObject pai{{"cpf", QStringLiteral("01234567890")}};
    const auto inscricao = leit::lerInscricao(pai, "tomador");
    QVERIFY2(inscricao.has_value(), "CPF em texto com zero à frente foi recusado");
    const auto* cpf = std::get_if<nb::Cpf>(&*inscricao);
    QVERIFY(cpf != nullptr);
    QCOMPARE(cpf->texto(), QStringLiteral("01234567890"));
}

void TstLeitorInscricao::cnpjNumericoJsonRecusado()
{
    // Número JSON perde o zero da frente (01234567890107 vira 1234567890107): a leitura tem de recusar, não aceitar o número.
    const QJsonObject pai{{"cnpj", QJsonValue(qint64{1234567890107})}};
    const auto inscricao = leit::lerInscricao(pai, "emitente");
    QVERIFY(!inscricao.has_value());
    QCOMPARE(QString::fromStdString(inscricao.error().codigo()), QStringLiteral("entrada.tipo_invalido"));
}

void TstLeitorInscricao::cpfNumericoJsonRecusado()
{
    // Número JSON de CPF com zero à frente (01234567890 vira 1234567890): recusado com o mesmo código de tipo.
    const QJsonObject pai{{"cpf", QJsonValue(qint64{1234567890})}};
    const auto inscricao = leit::lerInscricao(pai, "tomador");
    QVERIFY(!inscricao.has_value());
    QCOMPARE(QString::fromStdString(inscricao.error().codigo()), QStringLiteral("entrada.tipo_invalido"));
}

void TstLeitorInscricao::cnpjECpfJuntosReprovam()
{
    const QJsonObject pai{{"cnpj", QStringLiteral("01234567890107")}, {"cpf", QStringLiteral("01234567890")}};
    const auto inscricao = leit::lerInscricao(pai, "emitente");
    QVERIFY(!inscricao.has_value());
    QCOMPARE(QString::fromStdString(inscricao.error().codigo()), QStringLiteral("entrada.campos_conflitantes"));
}

void TstLeitorInscricao::semCnpjNemCpfReprova()
{
    const QJsonObject pai;
    const auto inscricao = leit::lerInscricao(pai, "emitente");
    QVERIFY(!inscricao.has_value());
    QCOMPARE(QString::fromStdString(inscricao.error().codigo()), QStringLiteral("entrada.campo_ausente"));
}

QTEST_GUILESS_MAIN(TstLeitorInscricao)
#include "tst_leitor_inscricao.moc"
