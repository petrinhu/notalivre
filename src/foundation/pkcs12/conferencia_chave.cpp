// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conferência de par chave/certificado (NL-2.2, rodada 3). Ver conferencia_chave.h.
// Fase vermelha: o stub aceita qualquer par; o teste de fixtures diferentes tem de reprovar.
#include <nl/foundation/pkcs12/conferencia_chave.h>

namespace nl::foundation::pkcs12 {

bool chaveConfereComCertificado(const x509_st*, const evp_pkey_st*) noexcept {
    return true;
}

} // namespace nl::foundation::pkcs12
