// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponteiros RAII dos tipos do OpenSSL usados pela credencial (NL-2.2). Os tipos entram por nome opaco, sem
// incluir cabeçalhos do OpenSSL: X509 e EVP_PKEY são typedef de struct x509_st e evp_pkey_st no OpenSSL 3.
#ifndef NL_FOUNDATION_PKCS12_PONTEIROS_OSSL_H
#define NL_FOUNDATION_PKCS12_PONTEIROS_OSSL_H

#include <memory>

struct x509_st;
struct evp_pkey_st;

namespace nl::foundation::pkcs12 {

/// Libera com X509_free. Definido no .cpp.
struct LiberadorX509 {
    void operator()(x509_st* certificado) const noexcept;
};

/// Libera com EVP_PKEY_free. Definido no .cpp.
struct LiberadorChave {
    void operator()(evp_pkey_st* chave) const noexcept;
};

using CertificadoX509 = std::unique_ptr<x509_st, LiberadorX509>;
using ChavePrivada = std::unique_ptr<evp_pkey_st, LiberadorChave>;

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_PONTEIROS_OSSL_H
