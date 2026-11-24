// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comprimento de um ASN1_STRING, compartilhado pelos motores pkcs12 e xmldsig (NL-3.1m). O OpenSSL 4.1 depreciou
// ASN1_STRING_length (com -Werror não compila) e trouxe ASN1_STRING_get_length, que devolve size_t.
// Fonte: include/openssl/asn1.h.in no tag openssl-4.1.0 (OSSL_DEPRECATEDIN_4_1_FOR na declaração de ASN1_STRING_length);
// crypto/asn1/asn1_lib.c no mesmo tag. Ambas leem o campo length; a nova só difere para length negativo, que nenhum
// setter da API produz.
#ifndef NL_FOUNDATION_COMPRIMENTO_ASN1_H
#define NL_FOUNDATION_COMPRIMENTO_ASN1_H

#include <openssl/asn1.h>
#include <openssl/opensslv.h>

namespace nl::foundation {

// Bytes do conteúdo, sem o NUL de fim. OPENSSL_VERSION_NUMBER 0x40100000L é o OpenSSL 4.1.0 (layout 0xMNN00PP0L);
// abaixo disso, ASN1_STRING_get_length não existe (o Flatpak usa 3.5.9).
inline int comprimentoAsn1(const ASN1_STRING* texto) {
#if OPENSSL_VERSION_NUMBER >= 0x40100000L
    return static_cast<int>(ASN1_STRING_get_length(texto));
#else
    return ASN1_STRING_length(texto);
#endif
}

} // namespace nl::foundation

#endif // NL_FOUNDATION_COMPRIMENTO_ASN1_H
