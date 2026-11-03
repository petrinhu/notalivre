// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0313 (NL-2.10h, H1): não é permitido informar cTribMun quando o emitente é MEI (opSimpNac 2).
// Linha Excel 340 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD. Regime e cTribMun lidos da DPS.

#include <QtTest>

#include <nl/back/regras/regra_e0313.h>

#include "fabrica_dps.h"

using nl::back::RegraE0313;

class TstRegraE0313 : public QObject
{
    Q_OBJECT

private slots:
    void passaMeiSemCTribMun();
    void passaSimplesComCTribMun();
    void passaSimplesSemCTribMun();
    void reprovaMeiComCTribMun();
};

void TstRegraE0313::passaMeiSemCTribMun()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0313{}.verificar(*dps).has_value());
}

void TstRegraE0313::passaSimplesComCTribMun()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.cTribMun = std::string{"501"};
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0313{}.verificar(*dps).has_value());
}

void TstRegraE0313::passaSimplesSemCTribMun()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0313{}.verificar(*dps).has_value());
}

void TstRegraE0313::reprovaMeiComCTribMun()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.cTribMun = std::string{"501"};
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0313{}.verificar(*dps)), QStringLiteral("E0313"));
}

QTEST_GUILESS_MAIN(TstRegraE0313)
#include "tst_regra_e0313.moc"
