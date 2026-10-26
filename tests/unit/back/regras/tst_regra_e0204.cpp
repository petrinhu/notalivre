// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0204 (NL-1.5): retenção pelo tomador (tpRetISSQN 2) exige tomador identificado por CNPJ ou CPF.
// Linha Excel 258 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD: casos que passam (CNPJ e CPF) e que reprovam.

#include <QtTest>

#include <nl/back/regras/regra_e0204.h>

#include "fabrica_dps.h"

using nl::back::RegraE0204;

class TstRegraE0204 : public QObject
{
    Q_OBJECT

private slots:
    void passaSemRetencao();
    void passaComRetencaoETomadorCnpj();
    void passaComRetencaoETomadorCpf();
    void reprovaComRetencaoSemTomador();
    void reprovaComRetencaoETomadorComNif();
};

void TstRegraE0204::passaSemRetencao()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0204{}.verificar(*dps).has_value());
}

void TstRegraE0204::passaComRetencaoETomadorCnpj()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0204{}.verificar(*dps).has_value());
}

void TstRegraE0204::passaComRetencaoETomadorCpf()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::cpfComEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0204{}.verificar(*dps).has_value());
}

void TstRegraE0204::reprovaComRetencaoSemTomador()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::semTomador;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0204{}.verificar(*dps)), QStringLiteral("E0204"));
}

void TstRegraE0204::reprovaComRetencaoETomadorComNif()
{
    teste::Opcoes opcoes;
    opcoes.tpRetISSQN = "2";
    opcoes.tomador = teste::TipoTomador::nifComEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0204{}.verificar(*dps)), QStringLiteral("E0204"));
}

QTEST_GUILESS_MAIN(TstRegraE0204)
#include "tst_regra_e0204.moc"
