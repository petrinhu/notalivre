// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0202 (NL-1.5): CNPJ do tomador igual ao CNPJ do prestador, comparado por inteiro.
// Linha Excel 257 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD: caso que passa e caso que reprova.

#include <QtTest>

#include <nl/back/regras/regra_e0202.h>

#include "fabrica_dps.h"

using nl::back::RegraE0202;

class TstRegraE0202 : public QObject
{
    Q_OBJECT

private slots:
    void passaComCnpjsDiferentes();
    void reprovaComCnpjDoTomadorIgualAoPrestador();
};

void TstRegraE0202::passaComCnpjsDiferentes()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0202{}.verificar(*dps).has_value());
}

void TstRegraE0202::reprovaComCnpjDoTomadorIgualAoPrestador()
{
    teste::Opcoes opcoes;
    opcoes.cnpjTomador = opcoes.cnpjPrestador;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0202{}.verificar(*dps)), QStringLiteral("E0202"));
}

QTEST_GUILESS_MAIN(TstRegraE0202)
#include "tst_regra_e0202.moc"
