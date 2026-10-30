// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo de classificação (NL-2.5, L-34): tabela de códigos do Qt, sem rede. Um slot por regra (transporte.<nome>).
#include <nl/foundation/transporte/classificacao_erro.h>

#include <QNetworkReply>
#include <QtTest>

using namespace nl::foundation::transporte;

class TesteClassificacaoErro : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void tabelaDeCodigosDeRede();
    void faixaDeRespostaCompleta();
    void redirecionamentoEhSo3xx();
    void antesDoEnvioSoParaFalhaDeConexaoETls();
};

void TesteClassificacaoErro::initTestCase()
{
}

void TesteClassificacaoErro::cleanupTestCase()
{
}

void TesteClassificacaoErro::tabelaDeCodigosDeRede()
{
    struct Caso {
        QNetworkReply::NetworkError codigo;
        TipoErro tipo;
        CausaErro causa;
        const char* nome;
    };
    const Caso casos[] = {
        {QNetworkReply::ConnectionRefusedError, TipoErro::Transitorio, CausaErro::Conexao, "conexao recusada"},
        {QNetworkReply::RemoteHostClosedError, TipoErro::Transitorio, CausaErro::Conexao, "conexao fechada"},
        {QNetworkReply::HostNotFoundError, TipoErro::Transitorio, CausaErro::Conexao, "host nao encontrado"},
        {QNetworkReply::TemporaryNetworkFailureError, TipoErro::Transitorio, CausaErro::Conexao, "falha temporaria"},
        {QNetworkReply::NetworkSessionFailedError, TipoErro::Transitorio, CausaErro::Conexao, "sessao falhou"},
        {QNetworkReply::TimeoutError, TipoErro::Transitorio, CausaErro::Timeout, "timeout do Qt"},
        {QNetworkReply::SslHandshakeFailedError, TipoErro::Definitivo, CausaErro::Tls, "handshake recusado"},
        {QNetworkReply::ProtocolFailure, TipoErro::Definitivo, CausaErro::Protocolo, "falha de protocolo"},
        {QNetworkReply::UnknownNetworkError, TipoErro::Definitivo, CausaErro::Protocolo, "desconhecido"},
    };
    for (const Caso& caso : casos) {
        const Classificacao obtido = classificarErroDeRede(caso.codigo);
        QVERIFY2(obtido.tipo == caso.tipo, caso.nome);
        QVERIFY2(obtido.causa == caso.causa, caso.nome);
    }
}

void TesteClassificacaoErro::faixaDeRespostaCompleta()
{
    const QNetworkReply::NetworkError dentro[] = {
        QNetworkReply::NoError,
        QNetworkReply::ContentAccessDenied,
        QNetworkReply::ContentOperationNotPermittedError,
        QNetworkReply::ContentNotFoundError,
        QNetworkReply::AuthenticationRequiredError,
        QNetworkReply::ContentReSendError,
        QNetworkReply::ContentConflictError,
        QNetworkReply::ContentGoneError,
        QNetworkReply::UnknownContentError,
        QNetworkReply::ProtocolInvalidOperationError,
        QNetworkReply::InternalServerError,
        QNetworkReply::OperationNotImplementedError,
        QNetworkReply::ServiceUnavailableError,
        QNetworkReply::UnknownServerError,
    };
    const QNetworkReply::NetworkError fora[] = {
        QNetworkReply::ConnectionRefusedError,
        QNetworkReply::RemoteHostClosedError,
        QNetworkReply::HostNotFoundError,
        QNetworkReply::TimeoutError,
        QNetworkReply::SslHandshakeFailedError,
        QNetworkReply::ProtocolUnknownError,
        QNetworkReply::ProtocolFailure,
        QNetworkReply::UnknownNetworkError,
    };
    for (const QNetworkReply::NetworkError codigo : dentro) {
        QVERIFY2(ehCodigoDeRespostaCompleta(codigo), "código dentro da faixa foi recusado");
    }
    for (const QNetworkReply::NetworkError codigo : fora) {
        QVERIFY2(!ehCodigoDeRespostaCompleta(codigo), "código fora da faixa foi aceito como resposta");
    }
}

void TesteClassificacaoErro::redirecionamentoEhSo3xx()
{
    QVERIFY2(ehRedirecionamento(300), "300 é redirecionamento");
    QVERIFY2(ehRedirecionamento(301), "301 é redirecionamento");
    QVERIFY2(ehRedirecionamento(302), "302 é redirecionamento");
    QVERIFY2(ehRedirecionamento(307), "307 é redirecionamento");
    QVERIFY2(ehRedirecionamento(399), "399 é redirecionamento");
    QVERIFY2(!ehRedirecionamento(200), "200 não é redirecionamento");
    QVERIFY2(!ehRedirecionamento(299), "299 não é redirecionamento");
    QVERIFY2(!ehRedirecionamento(400), "400 não é redirecionamento");
}

void TesteClassificacaoErro::antesDoEnvioSoParaFalhaDeConexaoETls()
{
    // NL-2.8, DA-2.41: "antes do envio" é conexão recusada, nome sem resolução, falha temporária de rede, sessão e
    // handshake TLS (a recusa do certificado de cliente acontece na camada TLS). Queda de conexão e timeout podem ter
    // deixado os bytes do POST chegar, então não são antes do envio.
    struct Caso {
        QNetworkReply::NetworkError codigo;
        bool antes;
        const char* nome;
    };
    const Caso casos[] = {
        {QNetworkReply::ConnectionRefusedError, true, "conexao recusada"},
        {QNetworkReply::HostNotFoundError, true, "host nao encontrado"},
        {QNetworkReply::TemporaryNetworkFailureError, true, "falha temporaria"},
        {QNetworkReply::NetworkSessionFailedError, true, "sessao falhou"},
        {QNetworkReply::SslHandshakeFailedError, true, "handshake recusado"},
        {QNetworkReply::RemoteHostClosedError, false, "conexao fechada depois de aberta"},
        {QNetworkReply::TimeoutError, false, "timeout"},
        {QNetworkReply::ProtocolFailure, false, "falha de protocolo"},
    };
    for (const Caso& caso : casos) {
        QVERIFY2(classificarErroDeRede(caso.codigo).antesDoEnvio == caso.antes, caso.nome);
    }
}

QTEST_GUILESS_MAIN(TesteClassificacaoErro)
#include "tst_classificacao_erro.moc"
