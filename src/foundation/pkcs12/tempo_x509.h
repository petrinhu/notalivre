// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparação de validade de certificado com um instante, substituta de X509_cmp_time e X509_cmp_current_time, que o
// OpenSSL 4.0 depreciou (com -Werror não compila). Privado da Foundation de pkcs12 (NL-3.0g).
#ifndef NL_FOUNDATION_PKCS12_TEMPO_X509_H
#define NL_FOUNDATION_PKCS12_TEMPO_X509_H

#include <openssl/asn1.h>

#include <ctime>

namespace nl::foundation::pkcs12 {

// Regra do X509_cmp_time, depreciado no OpenSSL 4.0 (com -Werror não compila): -1 se o instante do certificado é anterior
// ou igual; 1 se posterior; 0 em erro ou fora do formato. Uma só fonte de 3.0 a 4.0: sem macro de versão.
// RFC 5280 trata notAfter como inclusivo; a igualdade continua vencida aqui (porte; decisão fica com o CISO).
inline int compararComInstante(const ASN1_TIME* tempo, std::time_t instante) {
    const int tipo = ASN1_STRING_type(tempo);
    const int tamanho = ASN1_STRING_length(tempo);
    if ((tipo != V_ASN1_UTCTIME || tamanho != 13) && (tipo != V_ASN1_GENERALIZEDTIME || tamanho != 15)) {
        return 0;
    }
    const unsigned char* dados = ASN1_STRING_get0_data(tempo);
    for (int i = 0; i < tamanho - 1; ++i) {
        if (dados[i] < '0' || dados[i] > '9') {
            return 0;
        }
    }
    if (dados[tamanho - 1] != 'Z') {
        return 0;
    }
    const int comparacao = ASN1_TIME_cmp_time_t(tempo, instante);
    if (comparacao == -2) {
        return 0;
    }
    return comparacao <= 0 ? -1 : 1;
}

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_TEMPO_X509_H
