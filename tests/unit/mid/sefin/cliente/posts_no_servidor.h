// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: quantos POST /nfse o servidor falso recebeu, pela rota de registro de teste.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_POSTS_NO_SERVIDOR_H
#define NL_TESTE_POSTS_NO_SERVIDOR_H

#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/transporte/transporte_https.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <chrono>

#include "tls_cliente_teste.h"

using namespace std::chrono_literals;
namespace fp = nl::foundation::pkcs12;
namespace ft = nl::foundation::transporte;

namespace {

// Quantos POST /nfse o servidor recebeu, pelo registro de teste. Usa um cliente com certificado, que o servidor exige.
int postsNoServidor(int porta, const fp::CredencialX509& cred)
{
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    ft::TransporteHttps transporte(base, configuracaoTls(cred));
    auto resposta = transporte.enviar(
        ft::RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/_teste/posts"), {}, {}});
    if (!resposta) {
        return -1;
    }
    return static_cast<int>(QJsonDocument::fromJson(resposta->corpo).array().size());
}

} // namespace

#endif // NL_TESTE_POSTS_NO_SERVIDOR_H
