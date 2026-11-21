// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de não-compilação da Foundation sql (NL-3.1a, bloco U-R, CONTRACT 12.1): consultaDeArrayLocal.

#include <nl/foundation/sql/executor.h>

namespace sql = nl::foundation::sql;



void caso(sql::Executor& e)

{

    const char texto[] = "SELECT 1"; auto consulta = sql::Consulta::de(texto); // NL-CASO

    (void)e;

}
