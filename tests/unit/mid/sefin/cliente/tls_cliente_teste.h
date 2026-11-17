// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: a configuração TLS de cliente montada a partir de uma credencial (transporte da NL-2.5).
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_TLS_CLIENTE_TESTE_H
#define NL_TESTE_TLS_CLIENTE_TESTE_H

#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/transporte/transporte_https.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <QByteArray>
#include <QSslCertificate>
#include <QSslKey>

#include "fixture_certificado_teste.h"

namespace fp = nl::foundation::pkcs12;
namespace ft = nl::foundation::transporte;

namespace {

// TLS de cliente a partir da credencial: certificado DER e chave PKCS#8 PEM, como o transporte da NL-2.5 espera.
ft::ConfiguracaoTls configuracaoTls(const fp::CredencialX509& cred)
{
    ft::ConfiguracaoTls tls;
    unsigned char* der = nullptr;
    const int tamanho = i2d_X509(cred.certificado(), &der);
    if (tamanho > 0 && der != nullptr) {
        tls.certificadoCliente = QSslCertificate(QByteArray(reinterpret_cast<const char*>(der), tamanho), QSsl::Der);
    }
    OPENSSL_free(der);
    BIO* saida = BIO_new(BIO_s_mem());
    if (saida != nullptr && PEM_write_bio_PKCS8PrivateKey(saida, cred.chavePrivada(), nullptr, nullptr, 0, nullptr,
                                                          nullptr) == 1) {
        char* dados = nullptr;
        const long n = BIO_get_mem_data(saida, &dados);
        tls.chaveCliente = QSslKey(QByteArray(dados, static_cast<qsizetype>(n)), QSsl::Rsa, QSsl::Pem);
    }
    BIO_free(saida);
    tls.autoridades = QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem);
    return tls;
}

} // namespace

#endif // NL_TESTE_TLS_CLIENTE_TESTE_H
