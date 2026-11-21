// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costura de teste (NL-3.1a, v1j): lê o contador de tentativas de conexão do motor. Alvo nl_foundation_sql_costura, que só
// os testes linkam; o produto não o linka.

#include "nl/foundation/sql/costura_conexao.h"
#include "nl/foundation/sql/estado.h"

namespace nl::foundation::sql::costura {

int chamadasDeConexao() noexcept { return detalhe::tentativasDeConexao(); }
void zerarChamadas() noexcept { detalhe::zerarTentativasDeConexao(); }

} // namespace nl::foundation::sql::costura
