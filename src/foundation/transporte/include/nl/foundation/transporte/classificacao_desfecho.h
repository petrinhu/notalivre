// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo do desfecho do Qt (NL-3.C, plano seção 1.7): o que a espera síncrona observou vira resultado do transporte. Sem
// rede: a função recebe o Desfecho já preenchido, e o teste de tabela cobre os ramos sem servidor.
#ifndef NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_DESFECHO_H
#define NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_DESFECHO_H

#include <nl/foundation/transporte/transporte_https.h>

#include <QByteArray>
#include <QNetworkReply>
#include <QString>

#include <expected>
#include <string>
#include <vector>

namespace nl::foundation::transporte {

// O que a espera síncrona observou. Preenchido pelos sinais do Qt durante o loop.
struct Desfecho {
    bool expirou = false;
    bool excedeuTeto = false;
    bool handshake = false;
    int sslErros = 0;
    // NL-2.10e (E4 complemento): códigos do QSslError::SslError (no máximo 16) e o que o handshake negociou, todos de conjunto fechado.
    std::vector<int> codigosSsl;
    std::string versaoTls = "nenhuma";
    std::string protocoloNegociado = "nenhum";
    QNetworkReply::NetworkError codigo = QNetworkReply::NoError;
    int status = 0;
    QByteArray corpo;
    QByteArray tipoConteudo;  // Content-Type da resposta, como veio no cabeçalho (NL-2.10c, emenda 1 item a)
    bool contentEncodingIdentidade = true;
    // Bytes do corpo que o Qt escreveu no socket (sinal uploadProgress). Zero: nada do POST saiu da máquina (DA-2.41).
    qint64 bytesEscritos = 0;
};

/// Erro do transporte com tipo, causa e mensagem fixa. Função pública para o transporte montar o erro de configuração.
[[nodiscard]] ErroTransporte erro(TipoErro tipo, CausaErro causa, QString mensagem);

/// Resultado do pedido a partir do desfecho observado, com os dados TLS e os bytes escritos no erro (DA-2.41, DA-2.75).
[[nodiscard]] std::expected<RespostaHttp, ErroTransporte> interpretarDesfecho(const Desfecho& d);

} // namespace nl::foundation::transporte

#endif // NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_DESFECHO_H
