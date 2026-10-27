// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0710 (NL-1.5): MEI nunca informa pTotTribSN (texto de 20/10, DA-1.8).
// Linha Excel 565 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0710.h>

#include "fabrica_dps.h"

using nl::back::RegraE0710;

class TstRegraE0710 : public QObject
{
    Q_OBJECT

private slots:
    void passaMeiComTotalIndicador();
    void reprovaMeiComPercentualSn();
    void passaMeEppComPercentualSn();
};

void TstRegraE0710::passaMeiComTotalIndicador()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0710{}.verificar(*dps).has_value());
}

void TstRegraE0710::reprovaMeiComPercentualSn()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.totTribSn = true;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0710{}.verificar(*dps)), QStringLiteral("E0710"));
}

void TstRegraE0710::passaMeEppComPercentualSn()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTribSn = true;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0710{}.verificar(*dps).has_value());
}

QTEST_GUILESS_MAIN(TstRegraE0710)
#include "tst_regra_e0710.moc"
