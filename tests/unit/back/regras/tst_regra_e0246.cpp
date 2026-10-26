// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da regra E0246 (NL-1.5): cPais do endereço no exterior do tomador existe na ISO2, é diferente de BR e não é ZZ.
// Linha Excel 284 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026 (texto novo, DA-1.8). TDD.

#include <QtTest>

#include <nl/back/regras/regra_e0246.h>

#include "catalogo_falso.h"
#include "fabrica_dps.h"

using nl::back::RegraE0246;

class TstRegraE0246 : public QObject
{
    Q_OBJECT

private slots:
    void passaComPaisExistenteDiferenteDeBr();
    void reprovaComBr();
    void reprovaComZz();
    void reprovaComPaisInexistente();
};

void TstRegraE0246::passaComPaisExistenteDiferenteDeBr()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::exteriorComPais;
    opcoes.cPais = "US";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QVERIFY(!RegraE0246{catalogo}.verificar(*dps).has_value());
}

void TstRegraE0246::reprovaComBr()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::exteriorComPais;
    opcoes.cPais = "BR";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QCOMPARE(teste::codigoDe(RegraE0246{catalogo}.verificar(*dps)), QStringLiteral("E0246"));
}

void TstRegraE0246::reprovaComZz()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::exteriorComPais;
    opcoes.cPais = "ZZ";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QCOMPARE(teste::codigoDe(RegraE0246{catalogo}.verificar(*dps)), QStringLiteral("E0246"));
}

void TstRegraE0246::reprovaComPaisInexistente()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::exteriorComPais;
    opcoes.cPais = "XX";
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    QCOMPARE(teste::codigoDe(RegraE0246{catalogo}.verificar(*dps)), QStringLiteral("E0246"));
}

QTEST_GUILESS_MAIN(TstRegraE0246)
#include "tst_regra_e0246.moc"
