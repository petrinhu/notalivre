// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transporte HTTPS com mTLS (NL-2.5). Ver transporte_https.h.
//
// Cada chamada cria o próprio QNetworkAccessManager: nada de estado global (política do projeto). O tempo limite é
// um QTimer nosso, não o do Qt, porque o código do reply por timeout varia entre versões (abort vira
// OperationCanceledError). A mensagem de erro nunca leva texto do Qt nem do OpenSSL: esses textos trazem a URL da
// requisição (chave de acesso, NSU) e o host. Sai só a descrição fixa da causa, o código numérico do Qt e o estado do
// handshake. Classificação da recusa do servidor depois do handshake: ADR-0005, seção 3, decisão do CISO. Usa só
// sinais estruturados (sinal encrypted, status ausente, código UnknownNetworkError), nunca o texto do alerta.

#include <nl/foundation/transporte/transporte_https.h>

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
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

// Descrição fixa por causa: nenhum texto vindo de fora entra na mensagem.
QString descricaoDaCausa(CausaErro causa)
{
    switch (causa) {
    case CausaErro::Timeout:
        return QStringLiteral("tempo limite esgotado");
    case CausaErro::Conexao:
        return QStringLiteral("conexão indisponível ou interrompida");
    case CausaErro::Tls:
        return QStringLiteral("handshake TLS recusado");
    case CausaErro::Protocolo:
        return QStringLiteral("resposta de rede inválida");
    case CausaErro::Configuracao:
        return QStringLiteral("configuração incompleta");
    }
    return QStringLiteral("erro de transporte");
}

QString mensagemDeRede(CausaErro causa, QNetworkReply::NetworkError codigo, bool handshake, int sslErros)
{
    return QStringLiteral("%1 (codigo Qt %2, handshake %3, erros SSL %4)")
        .arg(descricaoDaCausa(causa))
        .arg(static_cast<int>(codigo))
        .arg(handshake ? QStringLiteral("sim") : QStringLiteral("não"))
        .arg(sslErros);
}

// Transitório: conexão recusada, nome sem resolução, conexão cortada. Definitivo: handshake recusado e o resto.
ErroTransporte classificarErroDeRede(QNetworkReply::NetworkError codigo, bool handshake, int sslErros)
{
    switch (codigo) {
    case QNetworkReply::SslHandshakeFailedError:
        return erro(TipoErro::Definitivo, CausaErro::Tls,
                    mensagemDeRede(CausaErro::Tls, codigo, handshake, sslErros));
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
        return erro(TipoErro::Transitorio, CausaErro::Conexao,
                    mensagemDeRede(CausaErro::Conexao, codigo, handshake, sslErros));
    default:
        return erro(TipoErro::Definitivo, CausaErro::Protocolo,
                    mensagemDeRede(CausaErro::Protocolo, codigo, handshake, sslErros));
    }
}

// O Qt reporta respostas com status de conteúdo, protocolo e servidor (4xx e 5xx) nos códigos 201 a 499. Fora disso e
// de NoError, o erro é de rede ou de proxy, e status recebido com erro de rede não vale como resposta completa.
bool faixaDeRespostaCompleta(QNetworkReply::NetworkError codigo)
{
    const int n = static_cast<int>(codigo);
    return codigo == QNetworkReply::NoError || (n >= 201 && n <= 499);
}

// Content-Length confere com os bytes lidos. Com Content-Encoding, o cabeçalho diz o tamanho comprimido, e a conferência
// fica de fora.
bool corpoConfereComContentLength(const QNetworkReply* resposta, qsizetype lidos)
{
    const QVariant tamanho = resposta->header(QNetworkRequest::ContentLengthHeader);
    if (!tamanho.isValid() || resposta->hasRawHeader("Content-Encoding")) {
        return true;
    }
    return tamanho.toLongLong() == static_cast<qint64>(lidos);
}

// O pedido sai com Accept-Encoding: identity. Resposta com Content-Encoding diferente de identity é protocolo
// inesperado: o transporte não descomprime, e o chamador não recebe corpo que não pediu.
bool contentEncodingEhIdentity(const QNetworkReply* resposta)
{
    const QByteArray valor = resposta->rawHeader("Content-Encoding").trimmed().toLower();
    return valor.isEmpty() || valor == "identity";
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
    pedido.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    // identity explícito: o Qt não descomprime a resposta sozinho, e a conferência do Content-Length vale sempre.
    pedido.setRawHeader("Accept-Encoding", "identity");
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
    int sslErros = 0;
    QTimer tempo;
    tempo.setSingleShot(true);
    QObject::connect(&tempo, &QTimer::timeout, resposta, [&expirou, resposta] {
        expirou = true;
        resposta->abort();
    });
    QObject::connect(resposta, &QNetworkReply::encrypted, resposta, [&handshakeConcluido] {
        handshakeConcluido = true;
    });
    QObject::connect(resposta, &QNetworkReply::sslErrors, resposta, [&sslErros](const QList<QSslError>& erros) {
        sslErros += static_cast<int>(erros.size());
    });
    QEventLoop laco;
    QObject::connect(resposta, &QNetworkReply::finished, &laco, &QEventLoop::quit);
    tempo.start(static_cast<int>(urlBase_.tempoLimite.count()));
    laco.exec();
    tempo.stop();

    if (expirou) {
        return std::unexpected(erro(TipoErro::Transitorio, CausaErro::Timeout,
                                    mensagemDeRede(CausaErro::Timeout, resposta->error(), handshakeConcluido, sslErros)));
    }

    const QNetworkReply::NetworkError codigo = resposta->error();
    const QVariant statusAtributo = resposta->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int status = statusAtributo.isValid() ? statusAtributo.toInt() : 0;
    if (status > 0) {
        if (!contentEncodingEhIdentity(resposta)) {
            return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                        mensagemDeRede(CausaErro::Protocolo, codigo, handshakeConcluido, sslErros)));
        }
        QByteArray corpo = resposta->readAll();
        if (faixaDeRespostaCompleta(codigo) && corpoConfereComContentLength(resposta, corpo.size())) {
            return RespostaHttp{status, std::move(corpo)};
        }
        return std::unexpected(erro(TipoErro::Transitorio, CausaErro::Conexao,
                                    mensagemDeRede(CausaErro::Conexao, codigo, handshakeConcluido, sslErros)));
    }
    if (handshakeConcluido && codigo == QNetworkReply::UnknownNetworkError) {
        return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Tls,
                                    QStringLiteral("servidor recusou depois do handshake, provável certificado do cliente")));
    }
    return std::unexpected(classificarErroDeRede(codigo, handshakeConcluido, sslErros));
}

} // namespace nl::foundation::transporte
