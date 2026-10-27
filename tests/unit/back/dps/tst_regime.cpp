// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do regime do prestador (NL-1.4): opSimpNac só 2 e 3 (XSD de 20/10), regApTribSN 1 a 3, regEspTrib do XSD.

#include <nl/back/dps/op_simp_nac.h>
#include <nl/back/dps/prestador.h>
#include <nl/back/dps/reg_ap_trib_sn.h>
#include <nl/back/dps/reg_esp_trib.h>
#include <nl/back/dps/regime_tributacao.h>
#include <nl/back/identificacao/cnpj.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::Cnpj;
using nl::back::OpSimpNac;
using nl::back::Prestador;
using nl::back::RegApTribSN;
using nl::back::RegEspTrib;
using nl::back::RegimeTributacao;

class TstRegime : public QObject
{
    Q_OBJECT

private slots:
    void meiComOpSimpNac2();
    void meEppComOpSimpNac3();
    void naoOptanteAceito();
    void regApTribSNSoComOpSimpNac3();
    void opSimpNacQuatroRejeitado();
    void opSimpNacCincoRejeitado();
    void regApTribSNTresValores();
    void regEspTribValoresDoXsd();
};

void TstRegime::meiComOpSimpNac2()
{
    // MEI (opSimpNac 2) sem regApTribSN e com regEspTrib 0; o prestador com CNPJ recebe o regime.
    const auto opSimpNac = OpSimpNac::de(u"2");
    QVERIFY(opSimpNac.has_value());
    QVERIFY(opSimpNac->valor() == OpSimpNac::Valor::mei);

    const auto regEspTrib = RegEspTrib::de(u"0");
    QVERIFY(regEspTrib.has_value());
    const auto regime = teste::valorSeOk(RegimeTributacao::montar(*opSimpNac, std::nullopt, *regEspTrib));
    QVERIFY(regime.has_value());
    QVERIFY(!regime->regApTribSN().has_value());

    const auto cnpj = Cnpj::de(u"11.222.333/0001-81");
    QVERIFY(cnpj.has_value());
    const Prestador prestador = Prestador::montar(nl::back::Inscricao{*cnpj}, std::nullopt, std::nullopt, std::nullopt,
                                                  std::nullopt, std::nullopt, *regime);
    QVERIFY(prestador.regime() == *regime);
}

void TstRegime::meEppComOpSimpNac3()
{
    const auto opSimpNac = OpSimpNac::de(u"3");
    QVERIFY(opSimpNac.has_value());
    QVERIFY(opSimpNac->valor() == OpSimpNac::Valor::meEpp);
}

void TstRegime::naoOptanteAceito()
{
    // DA-1.16 (decisão do CTO): 1 (não optante) é o autônomo pessoa física do consumidor declarado. Aceito.
    const auto naoOptante = OpSimpNac::de(u"1");
    QVERIFY(naoOptante.has_value());
    QVERIFY(naoOptante->valor() == OpSimpNac::Valor::naoOptante);
}

void TstRegime::regApTribSNSoComOpSimpNac3()
{
    // DA-1.16: regApTribSN só existe com opSimpNac 3 (ME/EPP). Com 1 ou 2 presente é erro.
    const auto regEsp = RegEspTrib::de(u"0");
    const auto regAp = RegApTribSN::de(u"1");
    const auto naoOptante = OpSimpNac::de(u"1");
    const auto mei = OpSimpNac::de(u"2");
    const auto meEpp = OpSimpNac::de(u"3");
    QVERIFY(regEsp.has_value());
    QVERIFY(regAp.has_value());
    QVERIFY(naoOptante.has_value());
    QVERIFY(mei.has_value());
    QVERIFY(meEpp.has_value());
    QCOMPARE(teste::codigoDoErro(RegimeTributacao::montar(*naoOptante, *regAp, *regEsp)),
             QStringLiteral("regime.regapttribsn_sem_meepp"));
    QCOMPARE(teste::codigoDoErro(RegimeTributacao::montar(*mei, *regAp, *regEsp)),
             QStringLiteral("regime.regapttribsn_sem_meepp"));
    QVERIFY(RegimeTributacao::montar(*meEpp, *regAp, *regEsp).has_value());
    QVERIFY(RegimeTributacao::montar(*mei, std::nullopt, *regEsp).has_value());
}

void TstRegime::opSimpNacQuatroRejeitado()
{
    // Desvio nº 1: o XSD de 20/10 tem só 1, 2 e 3 (tiposSimples_v1.01.xsd:1014-1016); a NT 009 cita 4, que não está no leiaute de 20/10.
    QCOMPARE(teste::codigoDoErro(OpSimpNac::de(u"4")), QStringLiteral("regime.opsimpnac"));
}

void TstRegime::opSimpNacCincoRejeitado()
{
    QCOMPARE(teste::codigoDoErro(OpSimpNac::de(u"5")), QStringLiteral("regime.opsimpnac"));
}

void TstRegime::regApTribSNTresValores()
{
    // TSRegimeApuracaoSimpNac (tiposSimples_v1.01.xsd:1019-1032): 1, 2 e 3.
    QVERIFY(RegApTribSN::de(u"1").has_value());
    QVERIFY(RegApTribSN::de(u"2").has_value());
    QVERIFY(RegApTribSN::de(u"3").has_value());
    QCOMPARE(teste::codigoDoErro(RegApTribSN::de(u"4")), QStringLiteral("regime.regapttribsn"));
}

void TstRegime::regEspTribValoresDoXsd()
{
    // TSRegEspTrib (tiposSimples_v1.01.xsd:1049-1062): 0, 1, 2, 3, 4, 5, 6 e 9. O 7 não existe.
    for (const char16_t* valor : {u"0", u"1", u"2", u"3", u"4", u"5", u"6", u"9"}) {
        QVERIFY(RegEspTrib::de(valor).has_value());
    }
    QCOMPARE(teste::codigoDoErro(RegEspTrib::de(u"7")), QStringLiteral("regime.regesptrib"));
}

QTEST_GUILESS_MAIN(TstRegime)
#include "tst_regime.moc"
