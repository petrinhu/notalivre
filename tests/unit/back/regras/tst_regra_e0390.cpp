// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0390 (NL-1.5; NL-3.L): cTribNac do item 12 ou do subitem 17.10 da lista de serviços exige o grupo atvEvento.
// O app ainda não tem esse grupo, então a regra recusa os dois localmente, com a mensagem que diz isso (DA-1.20). Os itens 11 e
// 13 e os subitens 17.09 e 17.11 não disparam. Linha Excel 414 da aba RN DPS_NFS-e 1.01 do Anexo I de 24/11/2026. TDD.

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
    // NL-3.L: o Anexo I de 24/11/2026 estendeu a E0390 ao subitem 17.10.
    void subitem1710SemAtvEventoRejeitadoPelaE0390();
    void subitem1711NaoDisparaE0390();
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

void TstRegraE0390::subitem1710SemAtvEventoRejeitadoPelaE0390()
{
    for (const char* cTribNac : {"171001", "171099"}) {
        teste::Opcoes opcoes;
        opcoes.cTribNac = cTribNac;
        const auto dps = teste::dpsCom(opcoes);
        QVERIFY2(dps.has_value(), cTribNac);
        QCOMPARE(teste::codigoDe(RegraE0390{}.verificar(*dps)), QStringLiteral("E0390"));
    }
}

void TstRegraE0390::subitem1711NaoDisparaE0390()
{
    // Fronteira: só o subitem 17.10 do item 17 exige atvEvento; 17.09, 17.11 e o 17.02 do caso padrão passam.
    for (const char* cTribNac : {"170901", "171101", "170201"}) {
        teste::Opcoes opcoes;
        opcoes.cTribNac = cTribNac;
        const auto dps = teste::dpsCom(opcoes);
        QVERIFY2(dps.has_value(), cTribNac);
        QVERIFY2(!RegraE0390{}.verificar(*dps).has_value(), cTribNac);
    }
}

QTEST_GUILESS_MAIN(TstRegraE0390)
#include "tst_regra_e0390.moc"
