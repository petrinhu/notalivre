// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha. Os liberadores são definitivos (X509_free e EVP_PKEY_free); descartar() ainda não libera.
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

void CredencialX509::descartar() noexcept {}

} // namespace nl::foundation::pkcs12
