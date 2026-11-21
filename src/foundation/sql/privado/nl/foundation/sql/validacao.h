// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conferência de DadosConexao antes de qualquer socket (NL-3.1a, bloco U-R). Sem chamada do conector.

#ifndef NL_FOUNDATION_SQL_PRIVADO_VALIDACAO_H
#define NL_FOUNDATION_SQL_PRIVADO_VALIDACAO_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/dados_conexao.h"

#include <expected>

namespace nl::foundation::sql::detalhe {

[[nodiscard]] std::expected<void, nl::base::Erro> validarDadosConexao(const DadosConexao& dados);

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_VALIDACAO_H
