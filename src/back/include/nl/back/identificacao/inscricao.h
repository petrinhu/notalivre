// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Inscrição de quem emite ou de quem toma: CNPJ ou CPF (NL-1.1). Alias, sem comportamento próprio.

#ifndef NL_BACK_IDENTIFICACAO_INSCRICAO_H
#define NL_BACK_IDENTIFICACAO_INSCRICAO_H

#include <variant>

#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>

namespace nl::back {

/// Inscrição fiscal de uma pessoa jurídica (Cnpj) ou física (Cpf).
using Inscricao = std::variant<Cnpj, Cpf>;

} // namespace nl::back

#endif // NL_BACK_IDENTIFICACAO_INSCRICAO_H
