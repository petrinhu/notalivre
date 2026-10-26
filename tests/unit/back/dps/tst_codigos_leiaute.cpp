// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes dos códigos de formato do leiaute (NL-1.4): só o formato do XSD, sem tabela (a tabela é NL-5.6).

#include <nl/back/dps/chave_nfse.h>
#include <nl/back/dps/codigo_classificacao_tributaria.h>
#include <nl/back/dps/codigo_indicador_operacao.h>
#include <nl/back/dps/codigo_nbs.h>
#include <nl/back/dps/codigo_pais.h>
#include <nl/back/dps/codigo_situacao_tributaria.h>
#include <nl/back/dps/codigo_trib_nacional.h>
#include <nl/back/dps/telefone.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::ChaveNfse;
using nl::back::CodigoClassificacaoTributaria;
using nl::back::CodigoIndicadorOperacao;
using nl::back::CodigoPais;
using nl::back::CodigoSituacaoTributaria;
using nl::back::CodigoTribNacional;
using nl::back::Telefone;

class TstCodigosLeiaute : public QObject
{
    Q_OBJECT

private slots:
    // CST: [0-9]{3} (tiposSimples_v1.01.xsd:1810, TSRTCCodSitTrib).
    void cstTresDigitosAceito();
    void cstComDoisDigitosRejeitado();
    void cstComLetraRejeitada();

    // cClassTrib: [0-9]{6} (tiposSimples_v1.01.xsd:1821, TSRTCCodClassTrib).
    void cClassTribSeisDigitosAceito();
    void cClassTribComCincoDigitosRejeitado();

    // cIndOp: [0-9]{6} (tiposSimples_v1.01.xsd:1716, TSRTCCodIndOp).
    void cIndOpSeisDigitosAceito();
    void cIndOpComLetraRejeitado();

    // cTribNac: [0-9]{6} (tiposSimples_v1.01.xsd:1373, TSCodTribNac).
    void codigoTribNacionalSeisDigitos();

    // chSubstda e refNFSe: [0-9]{6}([0-9A-Z]{14})[0-9]{30}, 50 posições (tiposSimples_v1.01.xsd:206).
    void chaveNfseComCinquentaPosicoesAceita();
    void chaveNfseComUmaPosicaoAMaisRejeitada();

    // cPais: [A-Z]{2} (tiposSimples_v1.01.xsd:1362, TSCodPaisISO).
    void paisDuasLetrasMaiusculas();

    // cNBS: [0-9]{9} (tiposSimples_v1.01.xsd:1384, TSCodNBS; elemento em tiposComplexos_v1.01.xsd:1483).
    void cNbsNoveDigitos();

    // fone: [0-9]{6,20} (tiposSimples_v1.01.xsd:558, TSTelefone).
    void telefoneDeSeisAVinteDigitos();
};

void TstCodigosLeiaute::cstTresDigitosAceito()
{
    const auto cst = CodigoSituacaoTributaria::de(u"001");
    QVERIFY(cst.has_value());
    QCOMPARE(cst->texto(), QStringLiteral("001"));
}

void TstCodigosLeiaute::cstComDoisDigitosRejeitado()
{
    QCOMPARE(teste::codigoDoErro(CodigoSituacaoTributaria::de(u"00")), QStringLiteral("cst.formato"));
}

void TstCodigosLeiaute::cstComLetraRejeitada()
{
    QCOMPARE(teste::codigoDoErro(CodigoSituacaoTributaria::de(u"00A")), QStringLiteral("cst.formato"));
}

void TstCodigosLeiaute::cClassTribSeisDigitosAceito()
{
    const auto cClassTrib = CodigoClassificacaoTributaria::de(u"000001");
    QVERIFY(cClassTrib.has_value());
    QCOMPARE(cClassTrib->texto(), QStringLiteral("000001"));
}

void TstCodigosLeiaute::cClassTribComCincoDigitosRejeitado()
{
    QCOMPARE(teste::codigoDoErro(CodigoClassificacaoTributaria::de(u"00001")), QStringLiteral("cclasstrib.formato"));
}

void TstCodigosLeiaute::cIndOpSeisDigitosAceito()
{
    const auto cIndOp = CodigoIndicadorOperacao::de(u"010101");
    QVERIFY(cIndOp.has_value());
    QCOMPARE(cIndOp->texto(), QStringLiteral("010101"));
}

void TstCodigosLeiaute::cIndOpComLetraRejeitado()
{
    QCOMPARE(teste::codigoDoErro(CodigoIndicadorOperacao::de(u"01010A")), QStringLiteral("cindop.formato"));
}

void TstCodigosLeiaute::codigoTribNacionalSeisDigitos()
{
    const auto cTribNac = CodigoTribNacional::de(u"170201");
    QVERIFY(cTribNac.has_value());
    QCOMPARE(teste::codigoDoErro(CodigoTribNacional::de(u"17020")), QStringLiteral("cod_trib_nacional.formato"));
}

void TstCodigosLeiaute::chaveNfseComCinquentaPosicoesAceita()
{
    // 6 dígitos, 14 alfanuméricos (inscrição alfanumérica), 30 dígitos: 50 posições.
    const auto chave = ChaveNfse::de(u"123456" u"12ABC34501DE35" u"123456789012345678901234567890");
    QVERIFY(chave.has_value());
    QCOMPARE(chave->texto().size(), qsizetype{50});
}

void TstCodigosLeiaute::chaveNfseComUmaPosicaoAMaisRejeitada()
{
    QCOMPARE(teste::codigoDoErro(ChaveNfse::de(u"123456" u"12ABC34501DE35" u"1234567890123456789012345678901")),
             QStringLiteral("chave_nfse.formato"));
}

void TstCodigosLeiaute::paisDuasLetrasMaiusculas()
{
    const auto pais = CodigoPais::de(u"BR");
    QVERIFY(pais.has_value());
    QCOMPARE(teste::codigoDoErro(CodigoPais::de(u"br")), QStringLiteral("pais.formato"));
    QCOMPARE(teste::codigoDoErro(CodigoPais::de(u"BRA")), QStringLiteral("pais.formato"));
}

void TstCodigosLeiaute::cNbsNoveDigitos()
{
    QVERIFY(nl::back::CodigoNbs::de(u"101011000").has_value());
    QCOMPARE(teste::codigoDoErro(nl::back::CodigoNbs::de(u"12345678")), QStringLiteral("cnbs.formato"));
}

void TstCodigosLeiaute::telefoneDeSeisAVinteDigitos()
{
    QVERIFY(Telefone::de(u"123456").has_value());
    QVERIFY(Telefone::de(u"12345678901234567890").has_value());
    QCOMPARE(teste::codigoDoErro(Telefone::de(u"12345")), QStringLiteral("telefone.formato"));
    QCOMPARE(teste::codigoDoErro(Telefone::de(u"123456789012345678901")), QStringLiteral("telefone.formato"));
}

QTEST_GUILESS_MAIN(TstCodigosLeiaute)
#include "tst_codigos_leiaute.moc"
