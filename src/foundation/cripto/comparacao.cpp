// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparação em tempo constante (NL-3.2b). Provisório do commit de testes: devolve falso sempre.

#include <nl/foundation/cripto/comparacao.h>

namespace nl::foundation::cripto {

bool iguaisEmTempoConstante(QByteArrayView, QByteArrayView) noexcept
{
    return false;
}

} // namespace nl::foundation::cripto
