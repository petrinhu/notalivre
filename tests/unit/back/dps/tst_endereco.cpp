// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do endereço (NL-1.4): CEP numérico de 8 dígitos (TSCEP) e cEndPost de 1 a 11 (TSCodigoEndPostal).

#include <nl/back/dps/endereco.h>
#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/identificacao/cep.h>
#include <nl/back/identificacao/codigo_municipio.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::Cep;
using nl::back::CodigoEndPostal;
using nl::back::CodigoMunicipio;
using nl::back::CodigoPais;
using nl::back::EnderecoExterior;
using nl::back::EnderecoNacional;

class TstEndereco : public QObject
{
    Q_OBJECT

private slots:
    void cepNumericoDeOitoDigitos();
    void enderecoNacionalGuardaCepDeOitoDigitos();
    void codigoEndPostalDeUmAOnze();
    void enderecoExteriorComCEndPostDeUmCaractere();
};

void TstEndereco::cepNumericoDeOitoDigitos()
{
    // TSCEP (tiposSimples_v1.01.xsd:508): [0-9]{8}. Desde 20/10 o CEP é numérico (onda1_leiaute.md, 2.3).
    const auto cep = Cep::de(u"01001000");
    QVERIFY(cep.has_value());
    QCOMPARE(cep->texto(), QStringLiteral("01001000"));
    QCOMPARE(teste::codigoDoErro(Cep::de(u"0100100")), QStringLiteral("cep.tamanho"));
    QCOMPARE(teste::codigoDoErro(Cep::de(u"0100100A")), QStringLiteral("cep.formato"));
}

void TstEndereco::enderecoNacionalGuardaCepDeOitoDigitos()
{
    const auto municipio = CodigoMunicipio::de(u"4305108");
    const auto cep = Cep::de(u"01001000");
    QVERIFY(municipio.has_value());
    QVERIFY(cep.has_value());
    const EnderecoNacional nacional = EnderecoNacional::montar(*municipio, *cep);
    QCOMPARE(nacional.cep().texto(), QStringLiteral("01001000"));
}

void TstEndereco::codigoEndPostalDeUmAOnze()
{
    // TSCodigoEndPostal (tiposSimples_v1.01.xsd:511-520): de 1 a 11 caracteres (era 11 no Anexo de 09/02).
    QVERIFY(CodigoEndPostal::de(u"1", "endereco/endExt/cEndPost").has_value());
    QVERIFY(CodigoEndPostal::de(u"12345678901", "endereco/endExt/cEndPost").has_value());
    QCOMPARE(teste::codigoDoErro(CodigoEndPostal::de(u"", "endereco/endExt/cEndPost")), QStringLiteral("texto.tamanho"));
    QCOMPARE(teste::codigoDoErro(CodigoEndPostal::de(u"123456789012", "endereco/endExt/cEndPost")), QStringLiteral("texto.tamanho"));
}

void TstEndereco::enderecoExteriorComCEndPostDeUmCaractere()
{
    const auto pais = CodigoPais::de(u"US");
    const auto cEndPost = CodigoEndPostal::de(u"1", "endereco/endExt/cEndPost");
    const auto cidade = nl::back::Cidade::de(u"Miami", "endExt/xCidade");
    const auto estado = nl::back::EstadoProvRegiao::de(u"Florida", "endExt/xEstProvReg");
    QVERIFY(pais.has_value());
    QVERIFY(cEndPost.has_value());
    QVERIFY(cidade.has_value());
    QVERIFY(estado.has_value());
    const EnderecoExterior exterior = EnderecoExterior::montar(*pais, *cEndPost, *cidade, *estado);
    QCOMPARE(exterior.cEndPost().texto(), QStringLiteral("1"));
}

QTEST_GUILESS_MAIN(TstEndereco)
#include "tst_endereco.moc"
