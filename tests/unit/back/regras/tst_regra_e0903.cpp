// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0903 (NL-1.5), ramo do serviço: tpOper obrigatório para cTribNac 25.05, 15.09, 17.12 e 10.05.
// O ramo tpEnteGov não existe no modelo (lacuna, ver relatório). Com grupo IBS/CBS ausente, a leitura literal
// ("deve ser informado") dispara; a escolha está registrada no relatório. Linha Excel 574 da aba RN. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0903.h>

#include "fabrica_dps.h"

using nl::back::RegraE0903;

class TstRegraE0903 : public QObject
{
    Q_OBJECT

private slots:
    void passaServicoForaDaLista();
    void passaServicoDaListaComTpOper();
    void reprovaServicoDaListaSemTpOper();
    void reprovaServicoDaListaSemGrupoIbsCbs();
};

void TstRegraE0903::passaServicoForaDaLista()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0903{}.verificar(*dps).has_value());
}

void TstRegraE0903::passaServicoDaListaComTpOper()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "250501";
    opcoes.ibscbs = true;
    opcoes.tpOper = true;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!RegraE0903{}.verificar(*dps).has_value());
}

void TstRegraE0903::reprovaServicoDaListaSemTpOper()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "250501";
    opcoes.ibscbs = true;
    opcoes.tpOper = false;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0903{}.verificar(*dps)), QStringLiteral("E0903"));
}

void TstRegraE0903::reprovaServicoDaListaSemGrupoIbsCbs()
{
    teste::Opcoes opcoes;
    opcoes.cTribNac = "100501";
    opcoes.ibscbs = false;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QCOMPARE(teste::codigoDe(RegraE0903{}.verificar(*dps)), QStringLiteral("E0903"));
}

QTEST_GUILESS_MAIN(TstRegraE0903)
#include "tst_regra_e0903.moc"
