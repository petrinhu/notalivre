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
    // O endpoint decide o ambiente, não a flag (NL-2.9, item 1): host de Produção é Produção, e flag divergente é recusada.
    void ambienteVemDoEndpointNaoDaFlag();
    // Host parecido com o de Produção (sufixo, prefixo ou subdomínio) não é Produção: comparação exata do host (CISO, 22:48).
    void hostParecidoComProducaoRecusado();
    // Endpoint sem https é recusado antes de qualquer envio, mesmo com host conhecido (CISO, 22:48).
    void endpointSemTlsRecusado();
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

void TstPoliticaEmissao::ambienteVemDoEndpointNaoDaFlag()
{
    QCOMPARE(nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://sefin.nfse.gov.br/SefinNacional")).value(), Ambiente::Producao);
    QCOMPARE(nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://sefin.producaorestrita.nfse.gov.br/SefinNacional")).value(), Ambiente::ProducaoRestrita);
    QCOMPARE(nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://127.0.0.1:9443")).value(), Ambiente::Homologacao);
    const auto desconhecido = nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://exemplo.com.br/api"));
    QVERIFY2(!desconhecido.has_value(), "host fora da lista tem de ser recusado");
    QCOMPARE(QString::fromStdString(desconhecido.error().codigo()), QStringLiteral("emissao.endpoint_desconhecido"));
    // Flag homologacao com URL de Produção: recusa antes de assinar (o caso da triagem).
    const auto divergente = nl::mid::composicao::conferirFlagDoAmbiente(Ambiente::Homologacao, Ambiente::Producao);
    QVERIFY2(!divergente.has_value(), "flag que diverge do endpoint tem de ser recusada");
    QCOMPARE(QString::fromStdString(divergente.error().codigo()), QStringLiteral("emissao.ambiente_divergente"));
}

void TstPoliticaEmissao::hostParecidoComProducaoRecusado()
{
    const auto sufixo = nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://sefin.nfse.gov.br.exemplo.com/SefinNacional"));
    QVERIFY2(!sufixo.has_value(), "host com sufixo do de Produção não pode ser Produção");
    QCOMPARE(QString::fromStdString(sufixo.error().codigo()), QStringLiteral("emissao.endpoint_desconhecido"));
    const auto prefixo = nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("https://xsefin.nfse.gov.br/SefinNacional"));
    QVERIFY2(!prefixo.has_value(), "host com prefixo do de Produção não pode ser Produção");
    QCOMPARE(QString::fromStdString(prefixo.error().codigo()), QStringLiteral("emissao.endpoint_desconhecido"));
}

void TstPoliticaEmissao::endpointSemTlsRecusado()
{
    const auto semTls = nl::mid::composicao::ambienteDoEndpoint(QStringLiteral("http://sefin.nfse.gov.br/SefinNacional"));
    QVERIFY2(!semTls.has_value(), "endpoint http tem de ser recusado");
    QCOMPARE(QString::fromStdString(semTls.error().codigo()), QStringLiteral("emissao.endpoint_inseguro"));
}

QTEST_APPLESS_MAIN(TstPoliticaEmissao)
#include "tst_politica_emissao.moc"
