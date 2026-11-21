// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco U-R (NL-3.1a): a única chamada de conexão do motor entra no I-G. A costura de teste conta zero.

#include "nl/foundation/sql/costura_conexao.h"

namespace nl::foundation::sql::costura {

int chamadasDeConexao() noexcept { return 0; }
void zerarChamadas() noexcept {}

} // namespace nl::foundation::sql::costura
