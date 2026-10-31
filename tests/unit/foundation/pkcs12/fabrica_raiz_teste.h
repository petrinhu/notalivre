// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fábrica de raízes sintéticas para os testes da carga das raízes da ICP-Brasil (NL-2.13). Só em tests/: nunca é
// instalada, nunca entra no repositório de confiança, nunca vira âncora de produção.
#ifndef NL_TESTE_FABRICA_RAIZ_TESTE_H
#define NL_TESTE_FABRICA_RAIZ_TESTE_H

#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QString>

#include <ctime>
#include <memory>
#include <vector>

namespace nl::foundation::pkcs12::teste {

/// Raiz autoassinada P-256 com CA:TRUE e validade [inicio, fim] (UTC). Devolve o DER; vazio se o OpenSSL falhar.
inline QByteArray raizSinteticaDer(const char* nomeComum, std::time_t inicio, std::time_t fim) {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> chave(EVP_EC_gen("P-256"), &EVP_PKEY_free);
    std::unique_ptr<X509, decltype(&X509_free)> cert(X509_new(), &X509_free);
    if (!chave || !cert) {
        return {};
    }
    X509_set_version(cert.get(), 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1);
    ASN1_TIME_set(X509_getm_notBefore(cert.get()), inicio);
    ASN1_TIME_set(X509_getm_notAfter(cert.get()), fim);
    X509_NAME* nome = X509_get_subject_name(cert.get());
    X509_NAME_add_entry_by_txt(nome, "C", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("BR"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(nome, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(nomeComum), -1, -1, 0);
    X509_set_issuer_name(cert.get(), nome);
    X509_set_pubkey(cert.get(), chave.get());
    BASIC_CONSTRAINTS* restricoes = BASIC_CONSTRAINTS_new();
    if (restricoes == nullptr) {
        return {};
    }
    restricoes->ca = 1;
    const int ok = X509_add1_ext_i2d(cert.get(), NID_basic_constraints, restricoes, 1, X509V3_ADD_DEFAULT);
    BASIC_CONSTRAINTS_free(restricoes);
    if (ok != 1 || X509_sign(cert.get(), chave.get(), EVP_sha256()) <= 0) {
        return {};
    }
    const int tamanho = i2d_X509(cert.get(), nullptr);
    if (tamanho <= 0) {
        return {};
    }
    QByteArray der(tamanho, '\0');
    unsigned char* escrita = reinterpret_cast<unsigned char*>(der.data());
    i2d_X509(cert.get(), &escrita);
    return der;
}

/// Uma raiz do repositório de teste: nome do arquivo, DER e finalidade declarada no metadado.
struct RaizTeste {
    QString arquivo;
    QByteArray der;
    QString proposito;
};

/// Escreve os .der e um SHA256SUMS com o mesmo formato do repositório real. Devolve o caminho do manifesto, ou vazio.
inline QString escreverRepositorioTeste(const QString& diretorio, const std::vector<RaizTeste>& raizes) {
    QString manifesto = diretorio + QStringLiteral("/SHA256SUMS");
    QFile arquivoManifesto(manifesto);
    if (!arquivoManifesto.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    arquivoManifesto.write("# repositorio de teste (NL-2.13)\n");
    for (const RaizTeste& raiz : raizes) {
        QFile der(diretorio + QLatin1Char('/') + raiz.arquivo);
        if (!der.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return {};
        }
        der.write(raiz.der);
        der.close();
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(raiz.der, QCryptographicHash::Sha256).toHex());
        arquivoManifesto.write(QStringLiteral("# meta %1 proposito=%2 fonte=teste data=2026-10-30 url=local\n")
                                   .arg(raiz.arquivo, raiz.proposito).toUtf8());
        arquivoManifesto.write(QStringLiteral("%1  %2\n").arg(hash, raiz.arquivo).toUtf8());
    }
    arquivoManifesto.close();
    return manifesto;
}

} // namespace nl::foundation::pkcs12::teste

#endif // NL_TESTE_FABRICA_RAIZ_TESTE_H
