// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo de classificação dos códigos de erro do Qt (NL-2.5, L-34): puro, sem rede, testado por tabela de códigos.
#ifndef NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_ERRO_H
#define NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_ERRO_H

#include <nl/foundation/transporte/transporte_https.h>

#include <QNetworkReply>

namespace nl::foundation::transporte {

/// Tipo e causa de um erro de rede do Qt. Sem mensagem: a mensagem é montada à parte, com texto fixo.
struct Classificacao {
    TipoErro tipo;
    CausaErro causa;
};

/// Erro de rede sem status HTTP recebido: conexão, handshake, tempo limite do Qt e protocolo.
Classificacao classificarErroDeRede(QNetworkReply::NetworkError codigo);

/// Códigos que, com status HTTP recebido, ainda valem como resposta completa: NoError, conteúdo (201 a 207 e 299),
/// protocolo só 302 (é assim que o Qt entrega o 400 do servidor) e servidor (401 a 403 e 499). Os demais códigos com
/// status não são resposta completa: erro de rede, de proxy ou de protocolo (301, 399 etc.).
bool ehCodigoDeRespostaCompleta(QNetworkReply::NetworkError codigo);

/// Redirecionamento (3xx). O transporte não segue redirecionamento, e 3xx vira erro definitivo de protocolo.
bool ehRedirecionamento(int status);

} // namespace nl::foundation::transporte

#endif // NL_FOUNDATION_TRANSPORTE_CLASSIFICACAO_ERRO_H
