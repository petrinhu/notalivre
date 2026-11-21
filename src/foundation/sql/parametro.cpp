// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fábricas do Parametro (NL-3.1a, bloco U-G): o valor é copiado para dentro do Parametro. Sem SQL, sem conector.

#include "nl/foundation/sql/parametro.h"

#include <utility>

namespace nl::foundation::sql {

Parametro Parametro::nulo() { return Parametro(Nulo{}, false); }
Parametro Parametro::inteiro(qint64 valor) { return Parametro(valor, false); }
Parametro Parametro::texto(QByteArrayView utf8) { return Parametro(utf8.toByteArray(), false); }
Parametro Parametro::bytes(QByteArrayView dados) { return Parametro(dados.toByteArray(), true); }
Parametro Parametro::data(QDate dia) { return Parametro(dia, false); }

const Parametro::Valor& Parametro::valor() const noexcept { return valor_; }
bool Parametro::ehBytes() const noexcept { return bytes_; }

Parametro::Parametro(Valor valor, bool bytes) : valor_(std::move(valor)), bytes_(bytes) {}

} // namespace nl::foundation::sql
