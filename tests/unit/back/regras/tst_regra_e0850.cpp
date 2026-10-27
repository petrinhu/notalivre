// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0850 (NL-1.5): IBS/CBS só com competência a partir de 01/01/2026.
// Linha Excel 566 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0850.h>

#include "fabrica_dps.h"

using nl::back::RegraE0850;

class TstRegraE0850 : public QObject
{
    Q_OBJECT

private slots:
    void passaComIbsCbsEmCompetenciaDe2026();
    void passaSemGrupoIbsCbsEmCompetenciaDe2025();
    void reprovaComIbsCbsEmCompetenciaDe2025();
};

void TstRegraE0850::passaComIbsCbsEmCompetenciaDe2026()
{
    teste::Opcoes opcoes;
    opcoes.ibscbs = true;
    opcoes.dCompet = QDate{2026, 1, 1};
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0850{}.verificar(*dps).has_value());
}

void TstRegraE0850::passaSemGrupoIbsCbsEmCompetenciaDe2025()
{
    teste::Opcoes opcoes;
    opcoes.ibscbs = false;
    opcoes.dCompet = QDate{2025, 12, 31};
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0850{}.verificar(*dps).has_value());
}

void TstRegraE0850::reprovaComIbsCbsEmCompetenciaDe2025()
{
    teste::Opcoes opcoes;
    opcoes.ibscbs = true;
    opcoes.dCompet = QDate{2025, 12, 31};
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0850{}.verificar(*dps)), QStringLiteral("E0850"));
}

QTEST_GUILESS_MAIN(TstRegraE0850)
#include "tst_regra_e0850.moc"
