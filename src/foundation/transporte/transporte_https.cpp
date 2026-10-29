// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transporte HTTPS com mTLS (NL-2.5). Ver transporte_https.h.
//
// Cada chamada cria o próprio QNetworkAccessManager: nada de estado global (política do projeto). O tempo limite é
// um QTimer nosso, não o do Qt, porque o código do reply por timeout varia entre versões (abort vira
// OperationCanceledError). O erro de handshake é classificado só pelo código SSL explícito; o que o Qt devolve após
// um handshake que o servidor recusou depois (alerta de certificado) vai no detalhe da mensagem, medido e não
// adivinhado. A classificação dessa janela é decisão do coordenador (ver relatório da NL-2.5).

#include <nl/foundation/transporte/transporte_https.h>

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>
#include <QVariant>

namespace nl::foundation::transporte {

namespace {

ErroTransporte erro(TipoErro tipo, CausaErro causa, QString mensagem)
{
    return ErroTransporte{tipo, causa, std::move(mensagem)};
}

ErroTransporte configuracaoInvalida(QString motivo)
{
    return erro(TipoErro::Definitivo, CausaErro::Configuracao, std::move(motivo));
}

// O Qt reporta respostas HTTP com status de erro (4xx e 5xx) como NetworkError. Com status recebido, a resposta vale e
// quem decide o que fazer com o status é o chamador. Os códigos abaixo são só os de status HTTP.
bool ehErroDeStatusHttp(QNetworkReply::NetworkError codigo)
{
    switch (codigo) {
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ContentConflictError:
    case QNetworkReply::ContentGoneError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::InternalServerError:
    case QNetworkReply::OperationNotImplementedError:
    case QNetworkReply::ServiceUnavailableError:
    case QNetworkReply::UnknownServerError:
        return true;
    default:
        return false;
    }
}

// Transitório: conexão recusada, nome sem resolução, conexão cortada. Definitivo: handshake recusado e o resto.
ErroTransporte classificarErroDeRede(QNetworkReply::NetworkError codigo, QString detalhe)
{
    switch (codigo) {
    case QNetworkReply::SslHandshakeFailedError:
        return erro(TipoErro::Definitivo, CausaErro::Tls, std::move(detalhe));
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
        return erro(TipoErro::Transitorio, CausaErro::Conexao, std::move(detalhe));
    default:
        return erro(TipoErro::Definitivo, CausaErro::Protocolo, std::move(detalhe));
    }
}

// mTLS: autoridade de servidor só a da configuração (sem as do sistema), TLS 1.2 ou superior, verificação do servidor
// obrigatória (nunca ignoreSslErrors). Certificado de cliente entra só se a credencial veio.
QSslConfiguration configurarSsl(const ConfiguracaoTls& tls)
{
    QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
    cfg.setProtocol(QSsl::TlsV1_2OrLater);
    cfg.setPeerVerifyMode(QSslSocket::VerifyPeer);
    cfg.setCaCertificates(tls.autoridades);
    if (!tls.certificadoCliente.isNull() && !tls.chaveCliente.isNull()) {
        cfg.setLocalCertificate(tls.certificadoCliente);
        cfg.setPrivateKey(tls.chaveCliente);
    }
    return cfg;
}

} // namespace

TransporteHttps::TransporteHttps(UrlBase urlBase, ConfiguracaoTls tls)
    : urlBase_(std::move(urlBase)), tls_(std::move(tls)) {}

std::expected<RespostaHttp, ErroTransporte> TransporteHttps::enviar(const RequisicaoHttp& requisicao)
{
    if (urlBase_.tempoLimite <= std::chrono::milliseconds{0}) {
        return std::unexpected(configuracaoInvalida(QStringLiteral("tempo limite não positivo")));
    }
    if (!urlBase_.base.isValid() || urlBase_.base.scheme() != QLatin1String("https")) {
        return std::unexpected(configuracaoInvalida(QStringLiteral("URL base sem https válido")));
    }
    if (!urlBase_.base.path().isEmpty() && urlBase_.base.path() != QLatin1String("/")) {
        return std::unexpected(configuracaoInvalida(QStringLiteral("URL base com caminho: o caminho vai no prefixo")));
    }
    if (tls_.autoridades.isEmpty()) {
        return std::unexpected(configuracaoInvalida(QStringLiteral("sem autoridade de servidor na configuração")));
    }
    if (!requisicao.caminho.startsWith(QLatin1Char('/'))) {
        return std::unexpected(configuracaoInvalida(QStringLiteral("caminho sem barra inicial")));
    }

    QUrl url = urlBase_.base;
    url.setPath(urlBase_.prefixo + requisicao.caminho);
    QNetworkRequest pedido(url);
    pedido.setRedirectPolicy(QNetworkRequest::ManualRedirectPolicy);
    pedido.setSslConfiguration(configurarSsl(tls_));
    if (!requisicao.tipoConteudo.isEmpty()) {
        pedido.setHeader(QNetworkRequest::ContentTypeHeader, requisicao.tipoConteudo);
    }

    QNetworkAccessManager gerenciador;
    QNetworkReply* resposta = nullptr;
    if (requisicao.metodo == "GET") {
        resposta = gerenciador.get(pedido);
    } else if (requisicao.metodo == "HEAD") {
        resposta = gerenciador.head(pedido);
    } else if (requisicao.metodo == "POST") {
        resposta = gerenciador.post(pedido, requisicao.corpo);
    } else {
        return std::unexpected(configuracaoInvalida(QStringLiteral("método não suportado")));
    }

    // Espera síncrona: o loop só para no fim da resposta, e o timer garante o fim (abort no disparo).
    bool expirou = false;
    bool handshakeConcluido = false;
    QStringList errosSsl;
    QTimer tempo;
    tempo.setSingleShot(true);
    QObject::connect(&tempo, &QTimer::timeout, resposta, [&expirou, resposta] {
        expirou = true;
        resposta->abort();
    });
    QObject::connect(resposta, &QNetworkReply::encrypted, resposta, [&handshakeConcluido] {
        handshakeConcluido = true;
    });
    QObject::connect(resposta, &QNetworkReply::sslErrors, resposta, [&errosSsl](const QList<QSslError>& erros) {
        for (const QSslError& e : erros) {
            errosSsl << e.errorString();
        }
    });
    QEventLoop laco;
    QObject::connect(resposta, &QNetworkReply::finished, &laco, &QEventLoop::quit);
    tempo.start(static_cast<int>(urlBase_.tempoLimite.count()));
    laco.exec();
    tempo.stop();

    const QNetworkReply::NetworkError codigo = resposta->error();
    const QVariant statusAtributo = resposta->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int status = statusAtributo.isValid() ? statusAtributo.toInt() : 0;
    const QByteArray corpo = resposta->readAll();
    const QString detalhe = QStringLiteral("codigo=%1; handshake=%2; status=%3; bytes=%4; ssl=[%5]; msg=%6")
                                .arg(static_cast<int>(codigo))
                                .arg(handshakeConcluido ? QStringLiteral("sim") : QStringLiteral("não"))
                                .arg(status)
                                .arg(corpo.size())
                                .arg(errosSsl.join(QStringLiteral("; ")), resposta->errorString());

    if (expirou) {
        return std::unexpected(erro(TipoErro::Transitorio, CausaErro::Timeout,
                                    QStringLiteral("tempo limite esgotado; ") + detalhe));
    }
    if (status > 0 && (codigo == QNetworkReply::NoError || ehErroDeStatusHttp(codigo))) {
        return RespostaHttp{status, corpo};
    }
    return std::unexpected(classificarErroDeRede(codigo, detalhe));
}

} // namespace nl::foundation::transporte
