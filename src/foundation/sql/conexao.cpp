// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco I-R (NL-3.1a): a única chamada de conexão do motor entra no I-G. As funções da costura de teste devolvem
// valores de partida; os slots de integração falham antes de lê-los, porque o executor ainda não conecta.

#include "nl/foundation/sql/costura_conexao.h"

#include <optional>
#include <utility>

namespace nl::foundation::sql::costura {

int chamadasDeConexao() noexcept { return 0; }
void zerarChamadas() noexcept {}

unsigned long capacidadesDoCliente(const Executor&) { return 0UL; }
int opcaoLocalInfile(const Executor&) { return 0; }
char primeiroByteDaSenhaDoConector(const Executor&) { return '\0'; }
bool ponteiroDaSenhaNulo(const Executor&) { return true; }

std::expected<void, nl::base::Erro> reexecutarPreparadoSemVinculo(Executor&)
{
    return std::unexpected(nl::base::Erro("sql.nao_implementado", std::nullopt, "não implementado"));
}

} // namespace nl::foundation::sql::costura
