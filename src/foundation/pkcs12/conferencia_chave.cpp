// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conferência de par chave/certificado (NL-2.2, rodada 3). Ver conferencia_chave.h.
#include <nl/foundation/pkcs12/conferencia_chave.h>

#include <openssl/err.h>
#include <openssl/x509.h>

namespace nl::foundation::pkcs12 {

bool chaveConfereComCertificado(const x509_st* certificado, const evp_pkey_st* chave) noexcept {
    // X509_check_private_key devolve 1 quando o par confere; 0 ou erro é recusa. A fila de erros é esvaziada aqui.
    const int resultado = X509_check_private_key(certificado, chave);
    ERR_clear_error();
    return resultado == 1;
}

} // namespace nl::foundation::pkcs12
