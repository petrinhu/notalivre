// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de não-compilação da Foundation sql (NL-3.1a, bloco U-R, CONTRACT 12.1): dadosSemModo.

#include <nl/foundation/sql/executor.h>

namespace sql = nl::foundation::sql;



void caso(sql::Executor& e)

{

    auto dados = sql::DadosConexao{.host = QByteArray("127.0.0.1")}; // NL-CASO

    (void)e;

}
