// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costura de teste (NL-3.1a, v1j): o contador e a leitura da sessão do conector. O produto só tem o que está aqui
// e no motor; a leitura do contador (chamadasDeConexao e zerarChamadas) fica no alvo nl_foundation_sql_costura, que só
// os testes linkam. A declaração de st_mysql basta: o cabeçalho não exige o conector.

#ifndef NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
#define NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H

#include "nl/base/erro.h"

#include <expected>

struct st_mysql;

namespace nl::foundation::sql {
class Executor;
} // namespace nl::foundation::sql

namespace nl::foundation::sql::costura {

// Alvo nl_foundation_sql_costura (costura.cpp): só os testes linkam.
[[nodiscard]] int chamadasDeConexao() noexcept;
void zerarChamadas() noexcept;

// Motor (conexao.cpp): a sessão do conector do executor, só para a costura de teste.
[[nodiscard]] st_mysql* sessaoDoConector(const Executor& executor);

// Motor (conexao.cpp): reexecuta um preparado sem novo vínculo; tem de devolver sql.sem_vinculo (guarda do Preparado).
[[nodiscard]] std::expected<void, nl::base::Erro> reexecutarPreparadoSemVinculo(Executor& executor);

} // namespace nl::foundation::sql::costura

#endif // NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
