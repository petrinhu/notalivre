// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0390 (NL-1.5): cTribNac do item 12 da lista de serviços exige o grupo atvEvento. O app ainda não tem
// esse grupo, então a regra recusa o item 12 localmente, com a mensagem que diz isso (DA-1.20). Os itens 11 e 13 não disparam.
// Linha Excel 380 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0390.h>

#include "fabrica_dps.h"

using nl::back::RegraE0390;

class TstRegraE0390 : public QObject
{
    Q_OBJECT

private slots:
    void passaComItemQueNaoEhDoze();
    void passaNosItensOnzeETreze();
    void reprovaComItemDoze();
    void mensagemDizQueOAppAindaNaoTemOGrupoAtvEvento();
};

void TstRegraE0390::passaComItemQueNaoEhDoze()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0390{}.verificar(*dps).has_value());
}

void TstRegraE0390::passaNosItensOnzeETreze()
{
    for (const char* cTribNac : {"110101", "112001", "130101", "139999"}) {
        teste::Opcoes opcoes;
        opcoes.cTribNac = cTribNac;
        const auto dps = teste::dpsCom(opcoes);
        QVERIFY2(dps.has_value(), cTribNac);
        QVERIFY2(!RegraE0390{}.verificar(*dps).has_value(), cTribNac);
    }
}

void TstRegraE0390::reprovaComItemDoze()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "120101";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0390{}.verificar(*dps)), QStringLiteral("E0390"));
}

void TstRegraE0390::mensagemDizQueOAppAindaNaoTemOGrupoAtvEvento()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "120101";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const auto erro = RegraE0390{}.verificar(*dps);
    QVERIFY(erro.has_value());
    QVERIFY(QString::fromStdString(erro->mensagem()).contains(QStringLiteral("o app ainda não tem o grupo atvEvento")));
}

QTEST_GUILESS_MAIN(TstRegraE0390)
#include "tst_regra_e0390.moc"
