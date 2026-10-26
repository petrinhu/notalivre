// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários da série da DPS (NL-1.3). Regra pura, sem I/O. A faixa e as cinco posições vêm de
// tiposSimples_v1.01.xsd:54 (TSIdDPS: "DPS" + Cód.Mun. + tipo + inscrição + Série DPS (5) + Núm. DPS (15)).

#include <nl/back/dps/serie.h>

#include <QtTest>

using nl::back::Serie;

namespace {

QString codigo(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstSerie : public QObject
{
    Q_OBJECT

private slots:
    void serieUmAceitaEEscreveCincoPosicoes();
    void serie49999Aceita();
    void serieZeroRejeitada();
    void serie50000Rejeitada();
    void serieNegativaRejeitada();
};

void TstSerie::serieUmAceitaEEscreveCincoPosicoes()
{
    // Série mínima da faixa do plano (NL-1.3): 1. Exemplo do leiaute: série 1 vira "00001" (onda1_leiaute.md, seção 3).
    const auto serie = Serie::de(1);
    QVERIFY(serie.has_value());
    QCOMPARE(serie->texto(), QStringLiteral("00001"));
}

void TstSerie::serie49999Aceita()
{
    // Limite superior da faixa 1 a 49999 do plano (NL-1.3); cinco posições cabem em TSIdDPS (tiposSimples_v1.01.xsd:54).
    const auto serie = Serie::de(49999);
    QVERIFY(serie.has_value());
    QCOMPARE(serie->texto(), QStringLiteral("49999"));
}

void TstSerie::serieZeroRejeitada()
{
    // Plano, NL-1.3, teste serieZeroRejeitada: zero não é série válida.
    const auto serie = Serie::de(0);
    QVERIFY(!serie.has_value());
    QCOMPARE(codigo(serie.error()), QStringLiteral("serie.faixa"));
}

void TstSerie::serie50000Rejeitada()
{
    // Plano, NL-1.3, teste serie50000Rejeitada: 50000 estoura as cinco posições da faixa de Série DPS.
    const auto serie = Serie::de(50000);
    QVERIFY(!serie.has_value());
    QCOMPARE(codigo(serie.error()), QStringLiteral("serie.faixa"));
}

void TstSerie::serieNegativaRejeitada()
{
    // Número negativo nunca escreve posição de dígito válida.
    const auto serie = Serie::de(-1);
    QVERIFY(!serie.has_value());
    QCOMPARE(codigo(serie.error()), QStringLiteral("serie.faixa"));
}

QTEST_GUILESS_MAIN(TstSerie)
#include "tst_serie.moc"
