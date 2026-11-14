// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo do desfecho do Qt (NL-3.C, plano 1.7 e 4.1): todo ramo de erro preenche codigoQt e handshake; o código 99 sem
// handshake não vira TLS (mata o MC5); a mensagem fixa da DA-2.35 não muda; os dados TLS e os bytes chegam ao erro.
#include <nl/foundation/transporte/classificacao_desfecho.h>

#include <QNetworkReply>
#include <QtTest>

#include <vector>

using namespace nl::foundation::transporte;

class TesteClassificacaoDesfecho : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cadaRamoPreencheCodigoQtEHandshake();
    void codigo99SemHandshakeNaoViraTls();
    void codigo99ComHandshakeViraTlsComMensagemFixa();
    void dadosTlsEBytesVaoAoErro();
};

void TesteClassificacaoDesfecho::initTestCase()
{
}

void TesteClassificacaoDesfecho::cleanupTestCase()
{
}

void TesteClassificacaoDesfecho::cadaRamoPreencheCodigoQtEHandshake()
{
    struct Caso {
        const char* ramo;
        Desfecho desfecho;
    };
    std::vector<Caso> casos;

    Desfecho teto;
    teto.excedeuTeto = true;
    teto.codigo = QNetworkReply::RemoteHostClosedError;
    casos.push_back({"teto do corpo excedido", teto});

    Desfecho tempo;
    tempo.expirou = true;
    tempo.codigo = QNetworkReply::TimeoutError;
    casos.push_back({"tempo limite esgotado", tempo});

    Desfecho redirecionamento;
    redirecionamento.status = 302;
    redirecionamento.codigo = QNetworkReply::OperationCanceledError;
    casos.push_back({"status com redirecionamento", redirecionamento});

    Desfecho codificado;
    codificado.status = 200;
    codificado.contentEncodingIdentidade = false;
    codificado.codigo = QNetworkReply::ProtocolFailure;
    casos.push_back({"status com Content-Encoding fora de identity", codificado});

    Desfecho redeComStatus;
    redeComStatus.status = 200;
    redeComStatus.codigo = QNetworkReply::ConnectionRefusedError;
    casos.push_back({"status com codigo de rede de 1 a 99", redeComStatus});

    Desfecho outroCodigo;
    outroCodigo.status = 200;
    outroCodigo.codigo = QNetworkReply::ProtocolUnknownError;
    casos.push_back({"status com outro codigo", outroCodigo});

    Desfecho recusaDepoisDoHandshake;
    recusaDepoisDoHandshake.handshake = true;
    recusaDepoisDoHandshake.codigo = QNetworkReply::UnknownNetworkError;
    casos.push_back({"DA-2.35, recusa depois do handshake", recusaDepoisDoHandshake});

    Desfecho semStatusComHandshake;
    semStatusComHandshake.handshake = true;
    semStatusComHandshake.codigo = QNetworkReply::SslHandshakeFailedError;
    casos.push_back({"erro de rede sem status, com handshake", semStatusComHandshake});

    Desfecho semStatusSemHandshake;
    semStatusSemHandshake.codigo = QNetworkReply::ConnectionRefusedError;
    casos.push_back({"erro de rede sem status, sem handshake", semStatusSemHandshake});

    for (const Caso& caso : casos) {
        const auto resultado = interpretarDesfecho(caso.desfecho);
        QVERIFY2(!resultado.has_value(), caso.ramo);
        QVERIFY2(resultado.error().codigoQt == static_cast<int>(caso.desfecho.codigo), caso.ramo);
        QVERIFY2(resultado.error().handshake == caso.desfecho.handshake, caso.ramo);
    }
}

void TesteClassificacaoDesfecho::codigo99SemHandshakeNaoViraTls()
{
    // MC5: sem handshake, o código 99 é protocolo definitivo. Tirar "d.handshake &&" da linha da DA-2.35 faz este slot reprovar.
    Desfecho d;
    d.handshake = false;
    d.codigo = QNetworkReply::UnknownNetworkError;
    d.status = 0;
    const auto resultado = interpretarDesfecho(d);
    QVERIFY(!resultado.has_value());
    QVERIFY2(resultado.error().tipo == TipoErro::Definitivo, "tipo");
    QVERIFY2(resultado.error().causa == CausaErro::Protocolo, "causa");
    QCOMPARE(resultado.error().codigoQt, 99);
    QVERIFY2(!resultado.error().handshake, "handshake");
}

void TesteClassificacaoDesfecho::codigo99ComHandshakeViraTlsComMensagemFixa()
{
    Desfecho d;
    d.handshake = true;
    d.codigo = QNetworkReply::UnknownNetworkError;
    d.status = 0;
    const auto resultado = interpretarDesfecho(d);
    QVERIFY(!resultado.has_value());
    QVERIFY2(resultado.error().tipo == TipoErro::Definitivo, "tipo");
    QVERIFY2(resultado.error().causa == CausaErro::Tls, "causa");
    QCOMPARE(resultado.error().mensagem,
             QStringLiteral("servidor recusou depois do handshake, provável certificado do cliente"));
}

void TesteClassificacaoDesfecho::dadosTlsEBytesVaoAoErro()
{
    Desfecho d;
    d.handshake = true;
    d.codigo = QNetworkReply::UnknownNetworkError;
    d.codigosSsl = {10};
    d.versaoTls = "tls1.3";
    d.protocoloNegociado = "http/1.1";
    d.bytesEscritos = 7;
    const auto resultado = interpretarDesfecho(d);
    QVERIFY(!resultado.has_value());
    QVERIFY2(resultado.error().codigosSsl == std::vector<int>{10}, "codigosSsl");
    QCOMPARE(QString::fromStdString(resultado.error().versaoTls), QStringLiteral("tls1.3"));
    QCOMPARE(QString::fromStdString(resultado.error().protocoloNegociado), QStringLiteral("http/1.1"));
    QCOMPARE(resultado.error().bytesEscritos, static_cast<qint64>(7));
}

QTEST_APPLESS_MAIN(TesteClassificacaoDesfecho)
#include "tst_classificacao_desfecho.moc"
