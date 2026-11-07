// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Comparação em tempo constante (NL-3.2b, pedido do CISO, ciso item 8). Toda conferência de HMAC ou de valor de
// conferência passa por aqui, nunca por ==.

#ifndef NL_FOUNDATION_CRIPTO_COMPARACAO_H
#define NL_FOUNDATION_CRIPTO_COMPARACAO_H

#include <QByteArrayView>

namespace nl::foundation::cripto {

/// Verdadeiro se as duas sequências têm o mesmo tamanho e os mesmos bytes. Tamanhos diferentes dão falso.
[[nodiscard]] bool iguaisEmTempoConstante(QByteArrayView a, QByteArrayView b) noexcept;

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_COMPARACAO_H
