// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0532 (NL-1.5): todo cTribNac 99.XX.XX implica tribISSQN 4 (texto de 20/10, DA-1.8).
// Linha Excel 484 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0532.h>

#include "fabrica_dps.h"

using nl::back::RegraE0532;

class TstRegraE0532 : public QObject
{
    Q_OBJECT

private slots:
    void passaComServicoComIncidencia();
    void passaComCodigo99ETribIssqnQuatro();
    void reprovaComCodigo99ETribIssqnUm();
    void reprovaComQualquerCodigo99XX();
};

void TstRegraE0532::passaComServicoComIncidencia()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0532{}.verificar(*dps).has_value());
}

void TstRegraE0532::passaComCodigo99ETribIssqnQuatro()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "990101";
    opcoes.tribISSQN = "4";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0532{}.verificar(*dps).has_value());
}

void TstRegraE0532::reprovaComCodigo99ETribIssqnUm()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "990101";
    opcoes.tribISSQN = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0532{}.verificar(*dps)), QStringLiteral("E0532"));
}

void TstRegraE0532::reprovaComQualquerCodigo99XX()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "991199";
    opcoes.tribISSQN = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0532{}.verificar(*dps)), QStringLiteral("E0532"));
}

QTEST_GUILESS_MAIN(TstRegraE0532)
#include "tst_regra_e0532.moc"
