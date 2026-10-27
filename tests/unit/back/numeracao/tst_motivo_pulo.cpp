// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do motivo de pulo de numeração (NL-1.3). Regra pura, sem I/O.
// Plano, NL-1.3, teste puloSempreTemMotivo: pular sem motivo não compila (sem construtor padrão) e o motivo vazio é recusado em execução.

#include <nl/back/numeracao/motivo_pulo.h>

#include <type_traits>

#include <QtTest>

using nl::back::MotivoPulo;

// Compila-se só se MotivoPulo não puder ser criado sem texto. Pular sem motivo, portanto, não compila.
static_assert(!std::is_default_constructible_v<MotivoPulo>,
              "pular sem motivo não pode compilar: MotivoPulo não tem construtor padrão (NL-1.3)");

namespace {

QString codigo(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstMotivoPulo : public QObject
{
    Q_OBJECT

private slots:
    void motivoComTextoAceitoEPreservado();
    void motivoVazioRejeitado();
    void motivoSoComEspacosRejeitado();
};

void TstMotivoPulo::motivoComTextoAceitoEPreservado()
{
    const auto motivo = MotivoPulo::de(u"Falha antes do envio");
    QVERIFY(motivo.has_value());
    QCOMPARE(motivo->texto(), QStringLiteral("Falha antes do envio"));
}

void TstMotivoPulo::motivoVazioRejeitado()
{
    const auto motivo = MotivoPulo::de(u"");
    QVERIFY(!motivo.has_value());
    QCOMPARE(codigo(motivo.error()), QStringLiteral("pulo.motivo_vazio"));
}

void TstMotivoPulo::motivoSoComEspacosRejeitado()
{
    const auto motivo = MotivoPulo::de(u"   \t ");
    QVERIFY(!motivo.has_value());
    QCOMPARE(codigo(motivo.error()), QStringLiteral("pulo.motivo_vazio"));
}

QTEST_GUILESS_MAIN(TstMotivoPulo)
#include "tst_motivo_pulo.moc"
