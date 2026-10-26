// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do número da DPS (NL-1.3). Regra pura, sem I/O. Faixa de 1 a 999999999999999 (quinze posições,
// Núm. DPS (15) em tiposSimples_v1.01.xsd:54).

#include <nl/back/dps/numero_dps.h>

#include <cstdint>

#include <QtTest>

using nl::back::NumeroDps;

namespace {

QString codigo(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstNumeroDps : public QObject
{
    Q_OBJECT

private slots:
    void numeroUmAceitaEEscreveQuinzePosicoes();
    void numeroQuinzeDigitosAceito();
    void numeroZeroRejeitado();
    void numeroAcimaDeQuinzeDigitosRejeitado();
    void numeroNegativoRejeitado();
};

void TstNumeroDps::numeroUmAceitaEEscreveQuinzePosicoes()
{
    // Exemplo do leiaute: número 1 vira "000000000000001" (onda1_leiaute.md, seção 3).
    const auto numero = NumeroDps::de(1);
    QVERIFY(numero.has_value());
    QCOMPARE(numero->texto(), QStringLiteral("000000000000001"));
}

void TstNumeroDps::numeroQuinzeDigitosAceito()
{
    // Maior número com quinze dígitos: fecha a faixa 1 a 999999999999999 (NL-1.3).
    const auto numero = NumeroDps::de(999999999999999);
    QVERIFY(numero.has_value());
    QCOMPARE(numero->texto(), QStringLiteral("999999999999999"));
}

void TstNumeroDps::numeroZeroRejeitado()
{
    // Plano, NL-1.3: número zero não é número de DPS.
    const auto numero = NumeroDps::de(0);
    QVERIFY(!numero.has_value());
    QCOMPARE(codigo(numero.error()), QStringLiteral("numero.faixa"));
}

void TstNumeroDps::numeroAcimaDeQuinzeDigitosRejeitado()
{
    // 1000000000000000 tem dezesseis dígitos; a posição 16 não existe no leiaute (TSIdDPS, xsd:54).
    const auto numero = NumeroDps::de(std::int64_t{1000000000000000});
    QVERIFY(!numero.has_value());
    QCOMPARE(codigo(numero.error()), QStringLiteral("numero.faixa"));
}

void TstNumeroDps::numeroNegativoRejeitado()
{
    const auto numero = NumeroDps::de(-1);
    QVERIFY(!numero.has_value());
    QCOMPARE(codigo(numero.error()), QStringLiteral("numero.faixa"));
}

QTEST_GUILESS_MAIN(TstNumeroDps)
#include "tst_numero_dps.moc"
