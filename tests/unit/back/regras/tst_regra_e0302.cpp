// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0302 (NL-1.5): cLocPrestacao existe na tabela de municípios do IBGE. "0000000" (Águas Marítimas) é uma
// linha da TAB.LOC.GERAL e é CONSULTADA no port, como qualquer código (DA-1.10 (a)). Linha Excel 333 da aba RN. TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0302.h>

#include "catalogo_falso.h"
#include "fabrica_dps.h"

using nl::back::RegraE0302;

class TstRegraE0302 : public QObject
{
    Q_OBJECT

private slots:
    void passaComMunicipioExistente();
    void passaComAguasMaritimasQuandoOCatalogoTemA();
    void reprovaComAguasMaritimasQuandoOCatalogoNaoTemA();
    void reprovaComMunicipioInexistente();
};

void TstRegraE0302::passaComMunicipioExistente()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QVERIFY(!RegraE0302{catalogo}.verificar(*dps).has_value());
}

void TstRegraE0302::passaComAguasMaritimasQuandoOCatalogoTemA()
{
    teste::Opcoes opcoes;
    opcoes.cLocPrestacao = "0000000";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QVERIFY(!RegraE0302{catalogo}.verificar(*dps).has_value());
}

void TstRegraE0302::reprovaComAguasMaritimasQuandoOCatalogoNaoTemA()
{
    teste::Opcoes opcoes;
    opcoes.cLocPrestacao = "0000000";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo{false};
    QCOMPARE(teste::codigoDe(RegraE0302{catalogo}.verificar(*dps)), QStringLiteral("E0302"));
}

void TstRegraE0302::reprovaComMunicipioInexistente()
{
    teste::Opcoes opcoes;
    opcoes.cLocPrestacao = "9999999";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QCOMPARE(teste::codigoDe(RegraE0302{catalogo}.verificar(*dps)), QStringLiteral("E0302"));
}

QTEST_GUILESS_MAIN(TstRegraE0302)
#include "tst_regra_e0302.moc"
