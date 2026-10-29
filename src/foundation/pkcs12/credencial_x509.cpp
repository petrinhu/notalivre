// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Credencial A1 (NL-2.2). Liberadores definitivos: X509_free e EVP_PKEY_free.
#include <nl/foundation/pkcs12/credencial_x509.h>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <utility>

namespace nl::foundation::pkcs12 {

void LiberadorX509::operator()(x509_st* certificado) const noexcept {
    X509_free(certificado);
}

void LiberadorChave::operator()(evp_pkey_st* chave) const noexcept {
    EVP_PKEY_free(chave);
}

CredencialX509::CredencialX509(CertificadoX509 certificado, ChavePrivada chave, Inscricao inscricao,
                               bool ekuAusente) noexcept
    : certificado_(std::move(certificado)),
      chave_(std::move(chave)),
      inscricao_(std::move(inscricao)),
      ekuAusente_(ekuAusente) {}

void CredencialX509::descartar() noexcept {
    // A chave primeiro: é o segredo. A inscrição fica, porque não é segredo.
    chave_.reset();
    certificado_.reset();
}

} // namespace nl::foundation::pkcs12
