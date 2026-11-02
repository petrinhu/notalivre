// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do tipo CodigoTribMunicipal (NL-2.10h, H1): cTribMun (TCCodTribMun, tiposSimples_v1.01.xsd:568), 3 dígitos.
// 000 é recusado pela regra E0315 (Anexo I de 20/10/2026, aba RN DPS_NFS-e 1.01, linha Excel 342). TDD: escrito antes do tipo.

#include <QtTest>

#include <nl/back/dps/codigo_trib_municipal.h>

#include "ajudantes.h"

using nl::back::CodigoTribMunicipal;

class TstCodigoTribMunicipal : public QObject
{
    Q_OBJECT

private slots:
    void tresDigitosAceitaEPreservaOTexto();
    void zeroRecusaPelaRegraE0315();
    void doisDigitosRecusa();
    void quatroDigitosRecusa();
    void letraRecusa();
    void vazioRecusa();
    void igualdadePeloTexto();
};

void TstCodigoTribMunicipal::tresDigitosAceitaEPreservaOTexto()
{
    QVERIFY(CodigoTribMunicipal::de(u"501").has_value());
    QVERIFY(CodigoTribMunicipal::de(u"001").has_value());
    QVERIFY(CodigoTribMunicipal::de(u"999").has_value());
    QCOMPARE(CodigoTribMunicipal::de(u"050")->texto(), QStringLiteral("050"));
}

void TstCodigoTribMunicipal::zeroRecusaPelaRegraE0315()
{
    QCOMPARE(teste::codigoDoErro(CodigoTribMunicipal::de(u"000")), QStringLiteral("cod_trib_municipal.zero"));
}

void TstCodigoTribMunicipal::doisDigitosRecusa()
{
    QCOMPARE(teste::codigoDoErro(CodigoTribMunicipal::de(u"50")), QStringLiteral("cod_trib_municipal.formato"));
}

void TstCodigoTribMunicipal::quatroDigitosRecusa()
{
    QCOMPARE(teste::codigoDoErro(CodigoTribMunicipal::de(u"5011")), QStringLiteral("cod_trib_municipal.formato"));
}

void TstCodigoTribMunicipal::letraRecusa()
{
    QCOMPARE(teste::codigoDoErro(CodigoTribMunicipal::de(u"50A")), QStringLiteral("cod_trib_municipal.formato"));
}

void TstCodigoTribMunicipal::vazioRecusa()
{
    QCOMPARE(teste::codigoDoErro(CodigoTribMunicipal::de(u"")), QStringLiteral("cod_trib_municipal.formato"));
}

void TstCodigoTribMunicipal::igualdadePeloTexto()
{
    const auto a = CodigoTribMunicipal::de(u"501");
    const auto b = CodigoTribMunicipal::de(u"501");
    const auto c = CodigoTribMunicipal::de(u"502");
    QVERIFY(a.has_value() && b.has_value() && c.has_value());
    QVERIFY(*a == *b);
    QVERIFY(*a != *c);
}

QTEST_GUILESS_MAIN(TstCodigoTribMunicipal)
#include "tst_codigo_trib_municipal.moc"
