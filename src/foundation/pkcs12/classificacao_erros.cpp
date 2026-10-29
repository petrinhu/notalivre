// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classificação dos erros do OpenSSL (NL-2.2, revisão 3, A7). Ver classificacao_erros.h.
// Fase vermelha: a regra antiga comparava só o motivo, sem a biblioteca (achado A7 da revisão 2).
#include <nl/foundation/pkcs12/classificacao_erros.h>

#include <openssl/err.h>
#include <openssl/pkcs12err.h>

namespace nl::foundation::pkcs12::interno {

Falha classificarCodigos(const std::vector<unsigned long>& codigos) {
    bool mac = false;
    bool semSuporte = false;
    const int motivoUnsupported = ERR_GET_REASON(ERR_R_UNSUPPORTED);
    for (unsigned long codigo : codigos) {
        if (ERR_GET_LIB(codigo) == ERR_LIB_PKCS12 && ERR_GET_REASON(codigo) == PKCS12_R_MAC_VERIFY_FAILURE) {
            mac = true;
        }
        // Regra antiga (vermelho): compara só o motivo. A fase verde exige também a biblioteca EVP.
        if (ERR_GET_REASON(codigo) == motivoUnsupported) {
            semSuporte = true;
        }
    }
    if (mac) {
        return Falha::SenhaErrada;
    }
    if (semSuporte) {
        return Falha::Unsupported;
    }
    return Falha::Malformado;
}

std::vector<unsigned long> colherErrosOpenssl() {
    std::vector<unsigned long> codigos;
    for (unsigned long codigo = ERR_get_error(); codigo != 0; codigo = ERR_get_error()) {
        codigos.push_back(codigo);
    }
    return codigos;
}

} // namespace nl::foundation::pkcs12::interno
