// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costura só para teste (NL-3.1a, bloco U-R): conta as tentativas de conexão do processo. Recusa antes do socket
// deixa a contagem em zero. Incluída só por src/foundation/sql/*.cpp e pelos testes.

#ifndef NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
#define NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H

namespace nl::foundation::sql::costura {

[[nodiscard]] int chamadasDeConexao() noexcept;
void zerarChamadas() noexcept;

} // namespace nl::foundation::sql::costura

#endif // NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
