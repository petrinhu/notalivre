// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do grupo totTrib por regime e das faixas (NL-2.10j, J1; DA-2.99). A montagem (Dps::montar) recusa, antes de qualquer
// número, as formas que o regime não aceita (dps.tottrib_regime: E0712, E0713, E0710) e os valores fora da faixa (dps.tottrib_faixa:
// E0702 a E0704 e E0706 a E0708). Linhas Excel 555 a 565 da aba RN DPS_NFS-e 1.01 do Anexo I de 20/10/2026. TDD.

#include <QtTest>

#include <nl/back/dps/percentual_tributo.h>
#include <nl/back/dps/percentual_tributo_sn.h>

#include "fabrica_dps.h"

class TstTotTribRegime : public QObject
{
    Q_OBJECT

private slots:
    void meiComIndTotTribAceita();
    void meiComMonetarioAceita();
    void meiComPTotTribSnRecusaComE0710();
    void meEppComIndTotTribRecusaComE0712();
    void meEppComPTotTribSnAceita();
    void meEppComPTotTribAceita();
    void naoOptanteComIndTotTribRecusaComE0713();
    void naoOptanteComPTotTribSnRecusaComE0713();
    void naoOptanteComPTotTribAceita();
    void percentualAte100Aceita();
    void percentualAcimaDe100RecusaComFaixa();
    void valorIgualAoServicoAceita();
    void valorAcimaDoServicoRecusaComFaixa();
    void percentualSnAte9999Aceita();
    void percentualSnAcimaDe9999Recusa();
};

namespace {

/// Código e mensagem do erro de montagem do caso, ou falha do teste quando a montagem passou.
void verificaRecusaDeMontagem(const teste::Opcoes& opcoes, const QString& codigo, const QString& trechoDaMensagem)
{
    const auto erro = teste::erroDeMontagemCom(opcoes);
    QVERIFY2(erro.has_value(), "a montagem tinha de recusar a forma de totTrib");
    QCOMPARE(QString::fromStdString(erro->codigo()), codigo);
    QVERIFY2(QString::fromStdString(erro->mensagem()).contains(trechoDaMensagem), qPrintable(QString::fromStdString(erro->mensagem())));
}

} // namespace

void TstTotTribRegime::meiComIndTotTribAceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.totTrib = teste::FormaTotTrib::indicador;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::meiComMonetarioAceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.totTrib = teste::FormaTotTrib::monetario;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::meiComPTotTribSnRecusaComE0710()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "2";
    opcoes.totTrib = teste::FormaTotTrib::sn;
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_regime"), QStringLiteral("E0710"));
}

void TstTotTribRegime::meEppComIndTotTribRecusaComE0712()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::indicador;
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_regime"), QStringLiteral("E0712"));
}

void TstTotTribRegime::meEppComPTotTribSnAceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::sn;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::meEppComPTotTribAceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::percentual;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::naoOptanteComIndTotTribRecusaComE0713()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "1";
    opcoes.totTrib = teste::FormaTotTrib::indicador;
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_regime"), QStringLiteral("E0713"));
}

void TstTotTribRegime::naoOptanteComPTotTribSnRecusaComE0713()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "1";
    opcoes.totTrib = teste::FormaTotTrib::sn;
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_regime"), QStringLiteral("E0713"));
}

void TstTotTribRegime::naoOptanteComPTotTribAceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "1";
    opcoes.totTrib = teste::FormaTotTrib::percentual;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::percentualAte100Aceita()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::percentual;
    opcoes.pTotTribFed = "100.00";
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::percentualAcimaDe100RecusaComFaixa()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::percentual;
    opcoes.pTotTribFed = "100.01";
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_faixa"), QStringLiteral("E0706"));
}

void TstTotTribRegime::valorIgualAoServicoAceita()
{
    // vServ do caso é 100,00 (10000 centavos): igual ao serviço passa (E0702 é "menor ou igual").
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::monetario;
    opcoes.vTotTribFedCentavos = 10000;
    QVERIFY(teste::dpsCom(opcoes).has_value());
}

void TstTotTribRegime::valorAcimaDoServicoRecusaComFaixa()
{
    teste::Opcoes opcoes;
    opcoes.opSimpNac = "3";
    opcoes.totTrib = teste::FormaTotTrib::monetario;
    opcoes.vTotTribFedCentavos = 10001;
    verificaRecusaDeMontagem(opcoes, QStringLiteral("dps.tottrib_faixa"), QStringLiteral("E0702"));
}

void TstTotTribRegime::percentualSnAte9999Aceita()
{
    QVERIFY(nl::back::PercentualTributoSn::de(u"99.99").has_value());
    QVERIFY(nl::back::PercentualTributoSn::de(u"0").has_value());
}

void TstTotTribRegime::percentualSnAcimaDe9999Recusa()
{
    QCOMPARE(teste::codigoDoErro(nl::back::PercentualTributoSn::de(u"100.00")), QStringLiteral("percentual_sn.formato"));
}

QTEST_GUILESS_MAIN(TstTotTribRegime)
#include "tst_tottrib_regime.moc"
