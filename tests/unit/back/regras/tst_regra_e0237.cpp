// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0237 (NL-1.5): retenção pelo tomador (tpRetISSQN 2) exige o endereço nacional do tomador.
// Linha Excel 277 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026 (colunas K e N = V, DA-1.19). TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0237.h>

#include "fabrica_dps.h"

using nl::back::RegraE0237;

class TstRegraE0237 : public QObject
{
    Q_OBJECT

private slots:
    void passaComRetencaoEEnderecoNacional();
    void passaSemRetencaoSemEndereco();
    void reprovaComRetencaoSemEndereco();
    void reprovaComRetencaoEEnderecoNoExterior();
};

void TstRegraE0237::passaComRetencaoEEnderecoNacional()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0237{}.verificar(*dps).has_value());
}

void TstRegraE0237::passaSemRetencaoSemEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cnpjSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0237{}.verificar(*dps).has_value());
}

void TstRegraE0237::reprovaComRetencaoSemEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::cnpjSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0237{}.verificar(*dps)), QStringLiteral("E0237"));
}

void TstRegraE0237::reprovaComRetencaoEEnderecoNoExterior()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::exteriorComPais;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0237{}.verificar(*dps)), QStringLiteral("E0237"));
}

QTEST_GUILESS_MAIN(TstRegraE0237)
#include "tst_regra_e0237.moc"
