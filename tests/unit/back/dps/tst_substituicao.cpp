// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da substituição (NL-1.4): cMotivo com exatamente duas posições (TSCodJustSubst) e xMotivo de 15 a 255 (TSMotivo).

#include <nl/back/dps/substituicao.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::ChaveNfse;
using nl::back::CodigoJustificativaSubstituicao;
using nl::back::MotivoSubstituicao;
using nl::back::Substituicao;

class TstSubstituicao : public QObject
{
    Q_OBJECT

private slots:
    void cMotivoComDuasPosicoesAceito();
    void cMotivoComUmaPosicaoRejeitado();
    void cMotivoForaDaListaRejeitado();
    void xMotivoDeQuinzeADuzentosECinquentaECinco();
    void substituicaoMontadaComChaveValida();
};

void TstSubstituicao::cMotivoComDuasPosicoesAceito()
{
    // TSCodJustSubst (tiposSimples_v1.01.xsd:235-250): 01, 02, 03, 04, 05 e 99.
    const auto um = CodigoJustificativaSubstituicao::de(u"01");
    QVERIFY(um.has_value());
    QVERIFY(um->valor() == CodigoJustificativaSubstituicao::Valor::desenquadramentoSimplesNacional);
    QVERIFY(CodigoJustificativaSubstituicao::de(u"99").has_value());
}

void TstSubstituicao::cMotivoComUmaPosicaoRejeitado()
{
    // Antes de 20/10 o campo tinha 1 posição; agora tem 2 (onda1_leiaute.md, 2.3).
    QCOMPARE(teste::codigoDoErro(CodigoJustificativaSubstituicao::de(u"1")), QStringLiteral("substituicao.motivo"));
}

void TstSubstituicao::cMotivoForaDaListaRejeitado()
{
    QCOMPARE(teste::codigoDoErro(CodigoJustificativaSubstituicao::de(u"06")), QStringLiteral("substituicao.motivo"));
    QCOMPARE(teste::codigoDoErro(CodigoJustificativaSubstituicao::de(u"00")), QStringLiteral("substituicao.motivo"));
}

void TstSubstituicao::xMotivoDeQuinzeADuzentosECinquentaECinco()
{
    QVERIFY(MotivoSubstituicao::de(QString(15, QChar{'a'}), "subst/xMotivo").has_value());
    QVERIFY(MotivoSubstituicao::de(QString(255, QChar{'a'}), "subst/xMotivo").has_value());
    QCOMPARE(teste::codigoDoErro(MotivoSubstituicao::de(QString(14, QChar{'a'}), "subst/xMotivo")), QStringLiteral("texto.tamanho"));
    QCOMPARE(teste::codigoDoErro(MotivoSubstituicao::de(QString(256, QChar{'a'}), "subst/xMotivo")), QStringLiteral("texto.tamanho"));
}

void TstSubstituicao::substituicaoMontadaComChaveValida()
{
    const auto chave = ChaveNfse::de(u"123456" u"12ABC34501DE35" u"123456789012345678901234567890");
    const auto motivo = CodigoJustificativaSubstituicao::de(u"01");
    QVERIFY(chave.has_value());
    QVERIFY(motivo.has_value());
    const Substituicao substituicao = Substituicao::montar(*chave, *motivo, std::nullopt);
    QVERIFY(substituicao.cMotivo().valor() == CodigoJustificativaSubstituicao::Valor::desenquadramentoSimplesNacional);
    QVERIFY(!substituicao.xMotivo().has_value());
}

QTEST_GUILESS_MAIN(TstSubstituicao)
#include "tst_substituicao.moc"
