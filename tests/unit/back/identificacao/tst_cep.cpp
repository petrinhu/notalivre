// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do CEP (NL-1.1, TSCEP). Regra pura, sem I/O.

#include <nl/back/identificacao/cep.h>

#include <QtTest>

using nl::back::Cep;

class TstCep : public QObject
{
    Q_OBJECT

private slots:
    void cepPreservaZeroAEsquerda();
    void cepNaoCompletaZero();
    void cepFormatoRejeitado();
};

void TstCep::cepPreservaZeroAEsquerda()
{
    // Máscara com hífen sai sem hífen; o zero inicial continua.
    const auto comMascara = Cep::de(u"01001-000");
    QVERIFY(comMascara.has_value());
    QCOMPARE(comMascara->texto(), QStringLiteral("01001000"));

    const auto semMascara = Cep::de(u"01001000");
    QVERIFY(semMascara.has_value());
    QCOMPARE(semMascara->texto(), QStringLiteral("01001000"));
}

void TstCep::cepNaoCompletaZero()
{
    // 7 dígitos: erro de tamanho, sem completar zero à esquerda.
    const auto cep = Cep::de(u"1001000");
    QVERIFY(!cep.has_value());
    QCOMPARE(QString::fromStdString(cep.error().codigo()), QStringLiteral("cep.tamanho"));
}

void TstCep::cepFormatoRejeitado()
{
    const auto cep = Cep::de(u"0100A000");
    QVERIFY(!cep.has_value());
    QCOMPARE(QString::fromStdString(cep.error().codigo()), QStringLiteral("cep.formato"));
}

QTEST_GUILESS_MAIN(TstCep)
#include "tst_cep.moc"
