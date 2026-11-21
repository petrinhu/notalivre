// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de não-compilação da Foundation sql (NL-3.1a, bloco U-R, CONTRACT 12.1): parametroQString.

#include <nl/foundation/sql/executor.h>
#include <QString>

namespace sql = nl::foundation::sql;



void caso(sql::Executor& e)

{

    auto parametro = sql::Parametro::texto(QString("x")); // NL-CASO

    (void)e;

}
