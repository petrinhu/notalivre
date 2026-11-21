// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costura só para teste (NL-3.1a, blocos U-R e I-R): conta as tentativas de conexão do processo e lê o estado interno do
// Executor (capacidades do cliente, opção de arquivo local, ponteiro da senha do conector e reexecução de preparado).
// Recusa antes do socket deixa a contagem em zero. Incluída só por src/foundation/sql/*.cpp e pelos testes.

#ifndef NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
#define NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H

#include "nl/base/erro.h"

#include <expected>

namespace nl::foundation::sql {
class Executor;
} // namespace nl::foundation::sql

namespace nl::foundation::sql::costura {

[[nodiscard]] int chamadasDeConexao() noexcept;
void zerarChamadas() noexcept;

// Capacidades que o cliente anunciou ao servidor nesta sessão (CLIENT_*), lidas do próprio cliente.
[[nodiscard]] unsigned long capacidadesDoCliente(const Executor& executor);
// Valor de MYSQL_OPT_LOCAL_INFILE do cliente desta sessão.
[[nodiscard]] int opcaoLocalInfile(const Executor& executor);
// Primeiro byte da cópia da senha que o conector guarda em MYSQL::passwd, e se o ponteiro é nulo.
[[nodiscard]] char primeiroByteDaSenhaDoConector(const Executor& executor);
[[nodiscard]] bool ponteiroDaSenhaNulo(const Executor& executor);
// Reexecuta o último preparado da sessão sem novo vínculo: tem de devolver sql.sem_vinculo.
[[nodiscard]] std::expected<void, nl::base::Erro> reexecutarPreparadoSemVinculo(Executor& executor);

} // namespace nl::foundation::sql::costura

#endif // NL_FOUNDATION_SQL_PRIVADO_COSTURA_CONEXAO_H
