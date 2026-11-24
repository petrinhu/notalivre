// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de não-compilação do dinheiro (NL-3.1e, NF-19): valorDeDouble.

#include <nl/back/dinheiro/valor.h>

void caso()
{
    auto valor = nl::back::Valor::de(1050); // NL-CASO
    (void)valor;
}
