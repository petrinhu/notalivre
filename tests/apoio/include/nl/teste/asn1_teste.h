// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ajuste de conteúdo de ASN1_STRING para os testes (NL-3.1o). O OpenSSL 4.1 depreciou ASN1_STRING_set
// (OSSL_DEPRECATEDIN_4_1_FOR, include/openssl/asn1.h.in no tag openssl-4.1.0) e trouxe ASN1_STRING_set1_data, com size_t
// no comprimento. Fonte do comportamento: crypto/asn1/asn1_lib.c no mesmo tag. Para len >= 0, as duas copiam os bytes e
// devolvem 1 em sucesso; a única diferença é o NUL de fim que ASN1_STRING_set acrescenta ao buffer interno, fora do length.
#ifndef NL_TESTE_APOIO_ASN1_TESTE_H
#define NL_TESTE_APOIO_ASN1_TESTE_H

#include <openssl/asn1.h>
#include <openssl/opensslv.h>

#include <cstddef>

namespace nl::teste {

// Copia `len` bytes de `dados` para `texto`. Usado só com comprimento explícito e não negativo.
inline int ajustarAsn1(ASN1_STRING* texto, const char* dados, int len) {
#if OPENSSL_VERSION_NUMBER >= 0x40100000L
    return ASN1_STRING_set1_data(texto, reinterpret_cast<const unsigned char*>(dados), static_cast<std::size_t>(len));
#else
    return ASN1_STRING_set(texto, dados, len);
#endif
}

} // namespace nl::teste

#endif // NL_TESTE_APOIO_ASN1_TESTE_H
