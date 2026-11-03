// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Cadeia do certificado até as âncoras de fora (NL-2.2, decisão do CISO, item 1). Interno ao módulo.
#ifndef NL_FOUNDATION_PKCS12_VERIFICADOR_CADEIA_H
#define NL_FOUNDATION_PKCS12_VERIFICADOR_CADEIA_H

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/erro_credencial.h>
#include <nl/foundation/pkcs12/ponteiros_ossl.h>

#include <expected>
#include <vector>

struct x509_st;

namespace nl::foundation::pkcs12 {

/// Monta a cadeia com as intermediárias do .pfx e exige que ela chegue a uma âncora. Cadeia de outra AC ou
/// incompleta devolve CadeiaInvalida. A validade no tempo fica fora daqui (é o Vencido, checado antes).
[[nodiscard]] std::expected<void, ErroCredencial> verificarCadeia(x509_st* folha,
                                                                  const std::vector<CertificadoX509>& intermediarios,
                                                                  const Ancoras& ancoras);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_VERIFICADOR_CADEIA_H
