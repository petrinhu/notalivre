// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transporte HTTPS com mTLS (NL-2.5). Ver transporte_https.h.
//
// Cada chamada cria o próprio QNetworkAccessManager: nada de estado global (política do projeto). O tempo limite é
// um QTimer nosso, não o do Qt, porque o código do reply por timeout varia entre versões (abort vira
// OperationCanceledError). A mensagem de erro nunca leva texto do Qt nem do OpenSSL: esses textos trazem a URL da
// requisição (chave de acesso, NSU) e o host. Sai só a descrição fixa da causa, o código numérico do Qt e o estado do
// handshake. Recusa do servidor depois do handshake: DA-2.35 do plano (decisão do CISO, 29/10/2026), por sinais
// estruturados (sinal encrypted, status ausente, código UnknownNetworkError), nunca pelo texto do alerta.
// Classificação dos códigos do Qt: átomo em classificacao_erro.h. A conferência do tamanho do corpo é do parser do Qt:
// corpo truncado chega como RemoteHostClosedError, código 2, e a faixa de resposta completa o recusa.

#include <nl/foundation/transporte/transporte_https.h>

#include <nl/foundation/transporte/classificacao_erro.h>

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

#include <optional>

namespace nl::foundation::transporte {

namespace {

// Teto do tempo limite: 10 minutos. Acima disso a configuração é inválida, e o valor cabe no int do QTimer.
constexpr std::chrono::milliseconds kTempoLimiteMaximo{600000};

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

// Erro de rede sem status: a classificação é do átomo; aqui só se monta a mensagem fixa.
ErroTransporte erroDeRede(QNetworkReply::NetworkError codigo, bool handshake, int sslErros)
{
    const Classificacao classe = classificarErroDeRede(codigo);
    return erro(classe.tipo, classe.causa, mensagemDeRede(classe.causa, codigo, handshake, sslErros));
}

// mTLS: autoridade de servidor só a da configuração (sem as do sistema), TLS 1.2 ou superior, verificação do servidor
// obrigatória (nunca ignoreSslErrors). Certificado de cliente entra só se a credencial veio.
// Pendência: a cópia da chave dentro da conexão ainda não foi medida (ADR-0002, linha 73), e precisa de contêiner.
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

// Valida tudo antes de abrir qualquer conexão: configuração inválida nunca sai da máquina.
std::optional<ErroTransporte> validarPedido(const UrlBase& base, const ConfiguracaoTls& tls,
                                            const RequisicaoHttp& requisicao)
{
    if (base.tempoLimite <= std::chrono::milliseconds{0}) {
        return configuracaoInvalida(QStringLiteral("tempo limite não positivo"));
    }
    if (base.tempoLimite > kTempoLimiteMaximo) {
        return configuracaoInvalida(QStringLiteral("tempo limite acima do teto de 10 minutos"));
    }
    if (!base.base.isValid() || base.base.scheme() != QLatin1String("https")) {
        return configuracaoInvalida(QStringLiteral("URL base sem https válido"));
    }
    if (!base.base.path().isEmpty() && base.base.path() != QLatin1String("/")) {
        return configuracaoInvalida(QStringLiteral("URL base com caminho: o caminho vai no prefixo"));
    }
    if (!base.prefixo.isEmpty() && !base.prefixo.startsWith(QLatin1Char('/'))) {
        return configuracaoInvalida(QStringLiteral("prefixo sem barra inicial"));
    }
    if (!requisicao.caminho.startsWith(QLatin1Char('/'))) {
        return configuracaoInvalida(QStringLiteral("caminho sem barra inicial"));
    }
    if (requisicao.metodo != "GET" && requisicao.metodo != "HEAD" && requisicao.metodo != "POST") {
        return configuracaoInvalida(QStringLiteral("método não suportado"));
    }
    if (requisicao.tetoCorpo <= 0) {
        return configuracaoInvalida(QStringLiteral("teto do corpo não positivo"));
    }
    if (tls.autoridades.isEmpty()) {
        return configuracaoInvalida(QStringLiteral("sem autoridade de servidor na configuração"));
    }
    if (tls.certificadoCliente.isNull() != tls.chaveCliente.isNull()) {
        return configuracaoInvalida(QStringLiteral("certificado e chave de cliente vêm juntos"));
    }
    return std::nullopt;
}

// O pedido sai com Accept-Encoding: identity. O Qt não descomprime a resposta sozinho.
QNetworkRequest montarPedido(const UrlBase& base, const ConfiguracaoTls& tls, const RequisicaoHttp& requisicao)
{
    QUrl url = base.base;
    url.setPath(base.prefixo + requisicao.caminho);
    QNetworkRequest pedido(url);
    pedido.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    pedido.setRawHeader("Accept-Encoding", "identity");
    pedido.setSslConfiguration(configurarSsl(tls));
    if (!requisicao.tipoConteudo.isEmpty()) {
        pedido.setHeader(QNetworkRequest::ContentTypeHeader, requisicao.tipoConteudo);
    }
    return pedido;
}

// O que a espera síncrona observou. Preenchido pelos sinais do Qt durante o loop.
struct Desfecho {
    bool expirou = false;
    bool excedeuTeto = false;
    bool handshake = false;
    int sslErros = 0;
    QNetworkReply::NetworkError codigo = QNetworkReply::NoError;
    int status = 0;
    QByteArray corpo;
    bool contentEncodingIdentidade = true;
    // Bytes do corpo que o Qt escreveu no socket (sinal uploadProgress). Zero: nada do POST saiu da máquina (DA-2.41).
    qint64 bytesEscritos = 0;
};

// Content-Encoding diferente de identity na resposta é protocolo inesperado: o transporte não descomprime.
bool contentEncodingEhIdentity(const QNetworkReply* resposta)
{
    const QByteArray valor = resposta->rawHeader("Content-Encoding").trimmed().toLower();
    return valor.isEmpty() || valor == "identity";
}

QNetworkReply* despachar(QNetworkAccessManager& gerenciador, const QNetworkRequest& pedido,
                         const RequisicaoHttp& requisicao)
{
    if (requisicao.metodo == "GET") {
        return gerenciador.get(pedido);
    }
    if (requisicao.metodo == "HEAD") {
        return gerenciador.head(pedido);
    }
    return gerenciador.post(pedido, requisicao.corpo);
}

// Espera síncrona. O loop para no fim da resposta; o timer aborta no disparo; o teto do corpo aborta na leitura, e
// antes dela, se o Content-Length já anuncia mais que o teto. Todas as lambdas usam o Desfecho deste escopo.
Desfecho executar(QNetworkAccessManager& gerenciador, const QNetworkRequest& pedido, const RequisicaoHttp& requisicao,
                  std::chrono::milliseconds tempoLimite)
{
    Desfecho desfecho;
    QNetworkReply* resposta = despachar(gerenciador, pedido, requisicao);
    const qint64 teto = requisicao.tetoCorpo;

    QTimer tempo;
    tempo.setSingleShot(true);
    QObject::connect(&tempo, &QTimer::timeout, resposta, [&desfecho, resposta] {
        desfecho.expirou = true;
        resposta->abort();
    });
    QObject::connect(resposta, &QNetworkReply::encrypted, resposta, [&desfecho] {
        desfecho.handshake = true;
    });
    QObject::connect(resposta, &QNetworkReply::sslErrors, resposta, [&desfecho](const QList<QSslError>& erros) {
        desfecho.sslErros += static_cast<int>(erros.size());
    });
    QObject::connect(resposta, &QNetworkReply::metaDataChanged, resposta, [&desfecho, resposta, teto] {
        const QVariant tamanho = resposta->header(QNetworkRequest::ContentLengthHeader);
        if (tamanho.isValid() && tamanho.toLongLong() > teto) {
            desfecho.excedeuTeto = true;
            resposta->abort();
        }
    });
    QObject::connect(resposta, &QNetworkReply::readyRead, resposta, [&desfecho, resposta, teto] {
        desfecho.corpo.append(resposta->readAll());
        if (desfecho.corpo.size() > teto) {
            desfecho.excedeuTeto = true;
            resposta->abort();
        }
    });

    QObject::connect(resposta, &QNetworkReply::uploadProgress, resposta, [&desfecho](qint64 enviados, qint64) {
        if (enviados > desfecho.bytesEscritos) {
            desfecho.bytesEscritos = enviados;
        }
    });
    QEventLoop laco;
    QObject::connect(resposta, &QNetworkReply::finished, &laco, &QEventLoop::quit);
    tempo.start(static_cast<int>(tempoLimite.count()));
    laco.exec();
    tempo.stop();

    desfecho.codigo = resposta->error();
    const QVariant statusAtributo = resposta->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    desfecho.status = statusAtributo.isValid() ? statusAtributo.toInt() : 0;
    desfecho.corpo.append(resposta->readAll());
    desfecho.contentEncodingIdentidade = contentEncodingEhIdentity(resposta);
    return desfecho;
}

// Transforma o que o Qt observou no resultado do transporte. Ordem: teto do corpo, tempo limite, status recebido,
// e por fim erro sem status.
std::expected<RespostaHttp, ErroTransporte> classificarDesfecho(const Desfecho& d)
{
    if (d.excedeuTeto) {
        return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                    mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)));
    }
    if (d.expirou) {
        return std::unexpected(erro(TipoErro::Transitorio, CausaErro::Timeout,
                                    mensagemDeRede(CausaErro::Timeout, d.codigo, d.handshake, d.sslErros)));
    }
    if (d.status > 0) {
        if (ehRedirecionamento(d.status) || !d.contentEncodingIdentidade) {
            return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                        mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)));
        }
        if (ehCodigoDeRespostaCompleta(d.codigo)) {
            return RespostaHttp{d.status, d.corpo};
        }
        // Erro de rede (1 a 99) depois do status: corpo incompleto ou conexão cortada, nunca resposta.
        const int codigo = static_cast<int>(d.codigo);
        if (codigo >= 1 && codigo <= 99) {
            return std::unexpected(erro(TipoErro::Transitorio, CausaErro::Conexao,
                                        mensagemDeRede(CausaErro::Conexao, d.codigo, d.handshake, d.sslErros)));
        }
        return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                    mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)));
    }
    // Recusa do servidor depois do handshake: sinal encrypted, sem status, código UnknownNetworkError (DA-2.35).
    if (d.handshake && d.codigo == QNetworkReply::UnknownNetworkError) {
        return std::unexpected(erro(TipoErro::Definitivo, CausaErro::Tls,
                                    QStringLiteral("servidor recusou depois do handshake, provável certificado do cliente"));
    }
    return std::unexpected(erroDeRede(d.codigo, d.handshake, d.sslErros));
}

// Resultado do pedido, com os bytes escritos do corpo no erro: é o sinal que o cliente usa para decidir se consulta (DA-2.41).
std::expected<RespostaHttp, ErroTransporte> interpretar(const Desfecho& d)
{
    auto resultado = classificarDesfecho(d);
    if (!resultado) {
        resultado.error().bytesEscritos = d.bytesEscritos;
    }
    return resultado;
}

} // namespace

// A credencial do A1 entra por cópia: ConfiguracaoTls guarda a chave (QSslKey) dentro do transporte, e cada envio copia
// a chave para o QSslConfiguration da conexão, que morre no fim do envio. Por isso o TransporteHttps tem a vida da
// credencial que o montou: o adaptador do Mid o cria a partir da CredencialX509 e o destrói junto com ela.
TransporteHttps::TransporteHttps(UrlBase urlBase, ConfiguracaoTls tls)
    : urlBase_(std::move(urlBase)), tls_(std::move(tls)) {}

std::expected<RespostaHttp, ErroTransporte> TransporteHttps::enviar(const RequisicaoHttp& requisicao)
{
    if (auto invalido = validarPedido(urlBase_, tls_, requisicao)) {
        return std::unexpected(*invalido);
    }
    QNetworkAccessManager gerenciador;
    const Desfecho desfecho =
        executar(gerenciador, montarPedido(urlBase_, tls_, requisicao), requisicao, urlBase_.tempoLimite);
    return interpretar(desfecho);
}

} // namespace nl::foundation::transporte
