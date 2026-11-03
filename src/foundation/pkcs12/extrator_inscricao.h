// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Extrai a inscrição do titular do otherName do SAN (NL-2.2). Interno ao módulo.
#ifndef NL_FOUNDATION_PKCS12_EXTRATOR_INSCRICAO_H
#define NL_FOUNDATION_PKCS12_EXTRATOR_INSCRICAO_H

#include <nl/foundation/pkcs12/inscricao.h>

#include <optional>

struct x509_st;

namespace nl::foundation::pkcs12 {

/// CNPJ no otherName 2.16.76.1.3.3; se não houver, CPF nas posições 9 a 19 do otherName 2.16.76.1.3.1.
/// Nunca usa o CN. nullopt se não houver otherName de identificação válido.
[[nodiscard]] std::optional<Inscricao> extrairInscricao(x509_st* certificado);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_EXTRATOR_INSCRICAO_H
