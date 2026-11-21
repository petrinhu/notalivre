// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do resultado de um MYSQL_STMT (NL-3.1a, bloco I-G): store, fetch e conversão para Valor. Truncamento (101)
// nunca é sucesso nem erro direto: a coluna com erro é relida com buffer do tamanho real. NUL: o motor usa o length.

#ifndef NL_FOUNDATION_SQL_PRIVADO_LEITURA_H
#define NL_FOUNDATION_SQL_PRIVADO_LEITURA_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/parametro.h"

#include <mysql.h>

#include <expected>

namespace nl::foundation::sql::detalhe {

// Buffer inicial por coluna, em bytes (política de leitura da NL-3.1a).
inline constexpr unsigned long kBufferInicialLeitura = 256;

[[nodiscard]] std::expected<Linhas, nl::base::Erro> lerLinhas(MYSQL_STMT* stmt);

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_LEITURA_H
