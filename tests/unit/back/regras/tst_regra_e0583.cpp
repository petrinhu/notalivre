// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0583 (NL-1.5): MEI (opSimpNac 2) não admite retenção do ISSQN (tpRetISSQN tem que ser 1).
// Linha Excel 513 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD. Regime lido do prestador da DPS.

#include <QtTest>

#include <nl/back/regras/regra_e0583.h>

#include "fabrica_dps.h"

using nl::back::RegraE0583;

class TstRegraE0583 : public QObject
{
    Q_OBJECT

private slots:
    void passaSimplesComRetencao();
    void passaMeiSemRetencao();
    void reprovaMeiComRetencaoPeloTomador();
};

void TstRegraE0583::passaSimplesComRetencao()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "1";
    opcoes.tpRetISSQN = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0583{}.verificar(*dps).has_value());
}

void TstRegraE0583::passaMeiSemRetencao()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.tpRetISSQN = "1";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0583{}.verificar(*dps).has_value());
}

void TstRegraE0583::reprovaMeiComRetencaoPeloTomador()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.tpRetISSQN = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0583{}.verificar(*dps)), QStringLiteral("E0583"));
}

QTEST_GUILESS_MAIN(TstRegraE0583)
#include "tst_regra_e0583.moc"
