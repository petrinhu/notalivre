// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conferência de par chave/certificado (NL-2.2, rodada 3, decisão do CISO item 2). Função própria (L-34), testada
// direto com chaves e certificados de fixtures diferentes. Os tipos do OpenSSL entram por declaração antecipada.
#ifndef NL_FOUNDATION_PKCS12_CONFERENCIA_CHAVE_H
#define NL_FOUNDATION_PKCS12_CONFERENCIA_CHAVE_H

struct x509_st;
struct evp_pkey_st;

namespace nl::foundation::pkcs12 {

/// Verdadeiro se a chave privada pertence ao certificado (mesmo par). Falso para par descasado ou erro.
[[nodiscard]] bool chaveConfereComCertificado(const x509_st* certificado, const evp_pkey_st* chave) noexcept;

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CONFERENCIA_CHAVE_H
