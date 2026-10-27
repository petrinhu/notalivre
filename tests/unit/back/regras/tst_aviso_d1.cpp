// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do aviso D1 (NL-1.5, ponto L10): tomador identificado por CNPJ sem endereço nacional.
// Aparece como aviso e não impede emitir (teste d1TomadorCnpjSemEnderecoAvisaENaoBloqueia do plano). TDD.

#include <QtTest>

#include <nl/back/regras/aviso_d1.h>
#include <nl/back/regras/validador_local.h>

#include "catalogo_falso.h"
#include "fabrica_dps.h"

using nl::back::AvisoD1;
using nl::back::ValidadorLocal;

class TstAvisoD1 : public QObject
{
    Q_OBJECT

private slots:
    void apareceParaTomadorCnpjSemEndereco();
    void naoApareceParaTomadorCnpjComEndereco();
    void naoApareceParaTomadorCpfSemEndereco();
    void d1TomadorCnpjSemEnderecoAvisaENaoBloqueia();
};

void TstAvisoD1::apareceParaTomadorCnpjSemEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cnpjSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const auto aviso = AvisoD1{}.avaliar(*dps);
    QVERIFY(aviso.has_value());
    QCOMPARE(QString::fromStdString(aviso->codigo()), QStringLiteral("D1"));
}

void TstAvisoD1::naoApareceParaTomadorCnpjComEndereco()
{
    const auto dps = teste::dpsCom(teste::Opcoes{});
    QVERIFY(dps.has_value());
    QVERIFY(!AvisoD1{}.avaliar(*dps).has_value());
}

void TstAvisoD1::naoApareceParaTomadorCpfSemEndereco()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cpfSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    QVERIFY(!AvisoD1{}.avaliar(*dps).has_value());
}

void TstAvisoD1::d1TomadorCnpjSemEnderecoAvisaENaoBloqueia()
{
    teste::Opcoes opcoes;
    opcoes.tomador = teste::TipoTomador::cnpjSemEndereco;
    const auto dps = teste::dpsCom(opcoes);
    QVERIFY(dps.has_value());
    const teste::CatalogoFalso catalogo;
    const auto resultado = ValidadorLocal::padrao(catalogo).validar(*dps);
    QVERIFY(resultado.erros.empty());
    QCOMPARE(resultado.avisos.size(), std::size_t{1});
    QCOMPARE(QString::fromStdString(resultado.avisos.front().codigo()), QStringLiteral("D1"));
    QVERIFY(resultado.podeEmitir());
}

QTEST_GUILESS_MAIN(TstAvisoD1)
#include "tst_aviso_d1.moc"
