// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do valor dhEmi (DA-1.27, decisão do CTO). Regra pura, sem I/O. O pattern do XSD é tiposSimples_v1.01.xsd:146
// (TSDateTimeUTC): ano de 2000 a 2099, offset de hora cheia de -11:00 a +12:00. Milissegundos são cortados, sem arredondar.
// Os valores esperados são literais escritos à mão; não derivam do próprio tipo.

#include <nl/back/dps/dh_emi.h>

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <QtTest>

using nl::back::DhEmi;

namespace {

/// Data e hora com offset fixo em segundos (-10800 é -03:00, 19800 é +05:30).
QDateTime dataHora(QDate data, QTime hora, int offsetSegundos)
{
    return QDateTime{data, hora, QTimeZone::fromSecondsAheadOfUtc(offsetSegundos)};
}

/// Texto do valor aceito, ou vazio se recusado (o teste compara o texto e o código, cada um sozinho).
QString textoDe(const QDateTime& entrada)
{
    const auto valor = DhEmi::de(entrada);
    return valor ? valor->texto() : QString{};
}

QString codigoDe(const QDateTime& entrada)
{
    const auto valor = DhEmi::de(entrada);
    return valor ? QString{} : QString::fromStdString(valor.error().codigo());
}

} // namespace

class TstDhEmi : public QObject
{
    Q_OBJECT

private slots:
    void offsetMenosTresAceitoEMantido();
    void offsetZeroEscritoComoMaisZero();
    void offsetMaisDozeAceito();
    void offsetMenosOnzeAceito();
    void offsetMaisCincoTrintaRecusado();
    void offsetMenosDozeRecusado();
    void offsetMaisTreceRecusado();
    void anoDoisMilCemRecusado();
    void anoMilNoventaNoveRecusado();
    void anosLimitesDoisMilEDoisMilNoventaNoveAceitos();
    void milissegundosCortadosSemArredondar();
    void dataInvalidaRecusada();
};

void TstDhEmi::offsetMenosTresAceitoEMantido()
{
    // O tipo nunca troca o offset: -03:00 entra e sai -03:00, sem conversão para UTC.
    QCOMPARE(textoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, -3 * 3600)), QStringLiteral("2026-10-25T10:00:00-03:00"));
}

void TstDhEmi::offsetZeroEscritoComoMaisZero()
{
    QCOMPARE(textoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, 0)), QStringLiteral("2026-10-25T10:00:00+00:00"));
}

void TstDhEmi::offsetMaisDozeAceito()
{
    QCOMPARE(textoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, 12 * 3600)), QStringLiteral("2026-10-25T10:00:00+12:00"));
}

void TstDhEmi::offsetMenosOnzeAceito()
{
    QCOMPARE(textoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, -11 * 3600)), QStringLiteral("2026-10-25T10:00:00-11:00"));
}

void TstDhEmi::offsetMaisCincoTrintaRecusado()
{
    // +05:30 não é hora cheia: o XSD recusa (o pattern só aceita :00).
    QCOMPARE(codigoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, 5 * 3600 + 30 * 60)), QStringLiteral("dhemi.formato"));
}

void TstDhEmi::offsetMenosDozeRecusado()
{
    // -12:00 está fora de -11:00 a +12:00 (TSDateTimeUTC).
    QCOMPARE(codigoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, -12 * 3600)), QStringLiteral("dhemi.formato"));
}

void TstDhEmi::offsetMaisTreceRecusado()
{
    // Fronteira um passo além de +12:00.
    QCOMPARE(codigoDe(dataHora(QDate{2026, 10, 25}, QTime{10, 0}, 13 * 3600)), QStringLiteral("dhemi.formato"));
}

void TstDhEmi::anoDoisMilCemRecusado()
{
    QCOMPARE(codigoDe(dataHora(QDate{2100, 1, 1}, QTime{10, 0}, -3 * 3600)), QStringLiteral("dhemi.formato"));
}

void TstDhEmi::anoMilNoventaNoveRecusado()
{
    // Fronteira um passo antes de 2000.
    QCOMPARE(codigoDe(dataHora(QDate{1999, 12, 31}, QTime{23, 59, 59}, -3 * 3600)), QStringLiteral("dhemi.formato"));
}

void TstDhEmi::anosLimitesDoisMilEDoisMilNoventaNoveAceitos()
{
    QCOMPARE(textoDe(dataHora(QDate{2000, 1, 1}, QTime{0, 0, 0}, -3 * 3600)), QStringLiteral("2000-01-01T00:00:00-03:00"));
    QCOMPARE(textoDe(dataHora(QDate{2099, 12, 31}, QTime{23, 59, 59}, -3 * 3600)), QStringLiteral("2099-12-31T23:59:59-03:00"));
}

void TstDhEmi::milissegundosCortadosSemArredondar()
{
    // 23:59:59,999 vira 23:59:59 do mesmo dia: arredondar iria para o dia seguinte e ultrapassaria dhProc (E0008, DA-1.27).
    QCOMPARE(textoDe(dataHora(QDate{2026, 10, 25}, QTime{23, 59, 59, 999}, -3 * 3600)), QStringLiteral("2026-10-25T23:59:59-03:00"));
}

void TstDhEmi::dataInvalidaRecusada()
{
    QCOMPARE(codigoDe(QDateTime{}), QStringLiteral("dhemi.formato"));
}

QTEST_GUILESS_MAIN(TstDhEmi)
#include "tst_dh_emi.moc"
