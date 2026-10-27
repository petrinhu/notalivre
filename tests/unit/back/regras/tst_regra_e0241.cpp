// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0241 (NL-1.5), ramo da retenção: com retenção pelo tomador, o tomador é CNPJ ou CPF e tem endereço nacional.
// O ramo da incidência no município do tomador é Onda 5 (DA-1.19). Linha Excel 278 da aba RN de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0241.h>

#include "fabrica_dps.h"

using nl::back::RegraE0241;

class TstRegraE0241 : public QObject
{
    Q_OBJECT

private slots:
    void passaSemRetencao();
    void passaComRetencaoETomadorCpfComEndereco();
    void reprovaComRetencaoETomadorCpfSemEndereco();
    void reprovaComRetencaoETomadorComNif();
};

void TstRegraE0241::passaSemRetencao()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cpfSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0241{}.verificar(*dps).has_value());
}

void TstRegraE0241::passaComRetencaoETomadorCpfComEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::cpfComEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0241{}.verificar(*dps).has_value());
}

void TstRegraE0241::reprovaComRetencaoETomadorCpfSemEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::cpfSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0241{}.verificar(*dps)), QStringLiteral("E0241"));
}

void TstRegraE0241::reprovaComRetencaoETomadorComNif()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::nifComEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0241{}.verificar(*dps)), QStringLiteral("E0241"));
}

QTEST_GUILESS_MAIN(TstRegraE0241)
#include "tst_regra_e0241.moc"
