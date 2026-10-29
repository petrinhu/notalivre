// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transporte HTTPS com mTLS (NL-2.5, plano). Motor da Foundation: depende só de base e de Qt (DA-2.30). A credencial do
// A1 entra como tipos Qt (certificado e chave); a conversão a partir do CredencialX509 da NL-2.2 é do adaptador no Mid.
// A URL base e o prefixo vêm sempre da configuração do ambiente (DA-2.5); nenhuma constante no código.
#ifndef NL_FOUNDATION_TRANSPORTE_TRANSPORTE_HTTPS_H
#define NL_FOUNDATION_TRANSPORTE_TRANSPORTE_HTTPS_H

#include <QByteArray>
#include <QList>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QUrl>

#include <chrono>
#include <expected>

namespace nl::foundation::transporte {

/// Transitório: vale tentar de novo (tempo esgotado, conexão recusada ou cortada). Definitivo: não adianta repetir.
enum class TipoErro {
    Transitorio,
    Definitivo,
};

enum class CausaErro {
    Timeout,        ///< O servidor não respondeu no tempo limite.
    Conexao,        ///< Não foi possível conectar, ou a conexão caiu.
    Tls,            ///< Handshake recusado (certificado de cliente ausente ou de outra AC, ou servidor não confiável).
    Protocolo,      ///< Resposta que o transporte não consegue ler.
    Configuracao,   ///< Configuração incompleta (sem autoridade de servidor, URL inválida).
};

/// Erro do transporte. A mensagem não traz URL com dados do titular nem o conteúdo da requisição.
struct ErroTransporte {
    TipoErro tipo;
    CausaErro causa;
    QString mensagem;
};

/// Endereço e política de tempo do ambiente (DA-2.5). O prefixo vazio significa sem prefixo.
struct UrlBase {
    QUrl base;
    QString prefixo;
    std::chrono::milliseconds tempoLimite{30000};
};

/// Credencial de cliente e autoridades do servidor. Sem autoridade, o transporte não envia nada.
struct ConfiguracaoTls {
    QSslCertificate certificadoCliente;   ///< Nulo = sem mTLS; o servidor exigente recusa o handshake.
    QSslKey chaveCliente;
    QList<QSslCertificate> autoridades;   ///< Âncoras de confiança do servidor.
};

struct RequisicaoHttp {
    QByteArray metodo;       ///< "GET", "POST" ou "HEAD".
    QString caminho;         ///< Relativo ao prefixo, com barra inicial, ex.: "/dps/ID".
    QByteArray corpo;
    QByteArray tipoConteudo;
    qint64 tetoCorpo = 16 * 1024 * 1024; ///< Teto do corpo da resposta, em bytes (padrão 16 MiB). Acima dele, erro definitivo de protocolo.
};

struct RespostaHttp {
    int status;
    QByteArray corpo;
};

class TransporteHttps {
public:
    TransporteHttps(UrlBase urlBase, ConfiguracaoTls tls);

    /// Síncrono: bloqueia a thread do chamador até a resposta, o erro ou o tempo limite. Não chamar da thread gráfica.
    [[nodiscard]] std::expected<RespostaHttp, ErroTransporte> enviar(const RequisicaoHttp& requisicao);

private:
    UrlBase urlBase_;
    ConfiguracaoTls tls_;
};

} // namespace nl::foundation::transporte

#endif // NL_FOUNDATION_TRANSPORTE_TRANSPORTE_HTTPS_H
