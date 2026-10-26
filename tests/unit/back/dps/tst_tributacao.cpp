// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da tributação do serviço (NL-1.4): tribISSQN, tpRetISSQN, totais aproximados e os padrões decimais do XSD.

#include <nl/back/dps/aliquota_issqn.h>
#include <nl/back/dps/indicador_total_tributos.h>
#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/percentual_tributo_sn.h>
#include <nl/back/dps/retencao_issqn.h>
#include <nl/back/dps/trib_issqn.h>
#include <nl/back/dps/tributacao.h>
#include <nl/back/dps/tributacao_municipal.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::IndicadorTotalTributos;
using nl::back::PercentualTributo;
using nl::back::PercentualTributoSn;
using nl::back::RetencaoIssqn;
using nl::back::TotalTributos;
using nl::back::Tributacao;
using nl::back::TributacaoMunicipal;
using nl::back::TribIssqn;

class TstTributacao : public QObject
{
    Q_OBJECT

private slots:
    void tribIssqnDeUmAQuatro();
    void tribIssqnDoisETresNaoSuportadosV1();
    void aliquotaIssqnFormatoDoXsd();
    void retencaoIssqnDeUmATres();
    void percentualTributoFormatoDoXsd();
    void percentualTributoSnFormatoDoXsd();
    void indicadorTotalTributosSoZero();
    void tributacaoComIndicadorDeTotalMontada();
};

void TstTributacao::tribIssqnDeUmAQuatro()
{
    // TSTribISSQN (tiposSimples_v1.01.xsd:1075): 1, 2, 3 e 4.
    for (const char16_t* valor : {u"1", u"2", u"3", u"4"}) {
        QVERIFY(TribIssqn::de(valor).has_value());
    }
    QCOMPARE(teste::codigoDoErro(TribIssqn::de(u"5")), QStringLiteral("tribissqn.valor"));
}

void TstTributacao::tribIssqnDoisETresNaoSuportadosV1()
{
    // DA-1.17: imunidade (2) e exportação (3) não são suportadas na v1 até o produto decidir (dps.nao_suportado_v1).
    const auto retencao = RetencaoIssqn::de(u"1");
    QVERIFY(retencao.has_value());
    for (const char16_t* valor : {u"2", u"3"}) {
        const auto issqn = TribIssqn::de(valor);
        QVERIFY(issqn.has_value());
        QCOMPARE(teste::codigoDoErro(TributacaoMunicipal::montar(*issqn, *retencao, std::nullopt)), QStringLiteral("dps.nao_suportado_v1"));
    }
    const auto operacao = TribIssqn::de(u"1");
    QVERIFY(operacao.has_value());
    QVERIFY(TributacaoMunicipal::montar(*operacao, *retencao, std::nullopt).has_value());
}

void TstTributacao::aliquotaIssqnFormatoDoXsd()
{
    // pAliq é TSDec1V2 (tiposSimples_v1.01.xsd:1461): 0 ou um dígito com até duas casas decimais; elemento em tiposComplexos_v1.01.xsd:2034.
    QVERIFY(nl::back::AliquotaIssqn::de(u"0").has_value());
    QVERIFY(nl::back::AliquotaIssqn::de(u"2.50").has_value());
    QVERIFY(nl::back::AliquotaIssqn::de(u"9.99").has_value());
    QCOMPARE(teste::codigoDoErro(nl::back::AliquotaIssqn::de(u"10.00")), QStringLiteral("aliquota_issqn.formato"));
    QCOMPARE(teste::codigoDoErro(nl::back::AliquotaIssqn::de(u"2.5")), QStringLiteral("aliquota_issqn.formato"));
}

void TstTributacao::retencaoIssqnDeUmATres()
{
    // TSTipoRetISSQN (tiposSimples_v1.01.xsd:1130): 1, 2 e 3. O zero não existe.
    for (const char16_t* valor : {u"1", u"2", u"3"}) {
        QVERIFY(RetencaoIssqn::de(valor).has_value());
    }
    QCOMPARE(teste::codigoDoErro(RetencaoIssqn::de(u"0")), QStringLiteral("retencao.valor"));
}

void TstTributacao::percentualTributoFormatoDoXsd()
{
    // TSDec3V2 (tiposSimples_v1.01.xsd:1479): 0 | 0.dd | [1-9][0-9]{0,2}(.dd)?.
    QVERIFY(PercentualTributo::de(u"0").has_value());
    QVERIFY(PercentualTributo::de(u"0.50").has_value());
    QVERIFY(PercentualTributo::de(u"999.99").has_value());
    QCOMPARE(teste::codigoDoErro(PercentualTributo::de(u"1234.50")), QStringLiteral("percentual.formato"));
    QCOMPARE(teste::codigoDoErro(PercentualTributo::de(u"1.5")), QStringLiteral("percentual.formato"));
    QCOMPARE(teste::codigoDoErro(PercentualTributo::de(u"05.00")), QStringLiteral("percentual.formato"));
}

void TstTributacao::percentualTributoSnFormatoDoXsd()
{
    // TSDec2V2 (tiposSimples_v1.01.xsd:1470): até dois dígitos inteiros.
    QVERIFY(PercentualTributoSn::de(u"99.99").has_value());
    QCOMPARE(teste::codigoDoErro(PercentualTributoSn::de(u"100")), QStringLiteral("percentual_sn.formato"));
}

void TstTributacao::indicadorTotalTributosSoZero()
{
    // TSTipoIndTotTrib (tiposSimples_v1.01.xsd:1268): só 0.
    QVERIFY(IndicadorTotalTributos::de(u"0").has_value());
    QCOMPARE(teste::codigoDoErro(IndicadorTotalTributos::de(u"1")), QStringLiteral("indtottrib.valor"));
}

void TstTributacao::tributacaoComIndicadorDeTotalMontada()
{
    const auto issqn = TribIssqn::de(u"1");
    const auto retencao = RetencaoIssqn::de(u"1");
    const auto indicador = IndicadorTotalTributos::de(u"0");
    QVERIFY(issqn.has_value());
    QVERIFY(retencao.has_value());
    QVERIFY(indicador.has_value());
    const auto municipal = teste::valorSeOk(TributacaoMunicipal::montar(*issqn, *retencao, std::nullopt));
    QVERIFY(municipal.has_value());
    const Tributacao tributacao = Tributacao::montar(*municipal, TotalTributos{*indicador});
    QVERIFY(std::holds_alternative<IndicadorTotalTributos>(tributacao.totTrib()));
}

QTEST_GUILESS_MAIN(TstTributacao)
#include "tst_tributacao.moc"
