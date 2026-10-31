// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre config` (NL-2.10a, passo 6). Ver comando_config.h. Stub do vermelho: ainda não grava nada.
#include "comando_config.h"

namespace nl::cli {

int executarConfig(const QStringList& argumentos)
{
    static_cast<void>(argumentos);
    return 2;
}

} // namespace nl::cli
