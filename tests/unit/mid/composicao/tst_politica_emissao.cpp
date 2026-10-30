// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da política de ambiente da emissão (NL-2.9, DA-2.5). Função pura: sem rede, sem arquivo.
#include <nl/mid/composicao/politica_emissao.h>

#include <QString>
#include <QtTest>

using nl::mid::composicao::Ambiente;
using nl::mid::composicao::ConfigEmissao;

class TstPoliticaEmissao : public QObject
{
    Q_OBJECT

private slots:
    // Produção sem liberação explícita é recusada com o código estável, antes de qualquer envio.
    void producaoBloqueadaPorPadrao();
    // Com a liberação do go/no-go, Produção passa; homologação e produção restrita passam sempre.
    void producaoPassaSoComLiberacao();
    // Texto desconhecido de ambiente é recusado.
    void ambienteDesconhecidoRecusado();
};

void TstPoliticaEmissao::producaoBloqueadaPorPadrao()
{
    const auto resultado = nl::mid::composicao::liberarAmbiente(ConfigEmissao{Ambiente::Producao, false});
    QVERIFY2(!resultado.has_value(), "Produção sem liberação tem de ser recusada");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("emissao.producao_bloqueada"));
}

void TstPoliticaEmissao::producaoPassaSoComLiberacao()
{
    QVERIFY(nl::mid::composicao::liberarAmbiente(ConfigEmissao{Ambiente::Producao, true}).has_value());
    QVERIFY(nl::mid::composicao::liberarAmbiente(ConfigEmissao{Ambiente::Homologacao, false}).has_value());
    QVERIFY(nl::mid::composicao::liberarAmbiente(ConfigEmissao{Ambiente::ProducaoRestrita, false}).has_value());
}

void TstPoliticaEmissao::ambienteDesconhecidoRecusado()
{
    const auto resultado = nl::mid::composicao::ambienteDeTexto("producao2");
    QVERIFY2(!resultado.has_value(), "ambiente fora da lista tem de ser recusado");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("emissao.ambiente_invalido"));
}

QTEST_APPLESS_MAIN(TstPoliticaEmissao)
#include "tst_politica_emissao.moc"
