// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de não-compilação da Foundation sql (NL-3.1a, bloco U-R, CONTRACT 12.1): consultaComAspas.

#include <nl/foundation/sql/executor.h>

namespace sql = nl::foundation::sql;



void caso(sql::Executor& e)

{

    auto consulta = sql::Consulta::de("SELECT a"); // NL-CASO

    (void)e;

}
