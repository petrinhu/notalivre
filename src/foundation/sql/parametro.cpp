// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco U-R (NL-3.1a): as fábricas entram no U-G. Até lá devolvem um Parametro nulo, que os testes recusam.

#include "nl/foundation/sql/parametro.h"

namespace nl::foundation::sql {

Parametro Parametro::nulo() { return Parametro(Nulo{}, false); }
Parametro Parametro::inteiro(qint64) { return Parametro(Nulo{}, false); }
Parametro Parametro::texto(QByteArrayView) { return Parametro(Nulo{}, false); }
Parametro Parametro::bytes(QByteArrayView) { return Parametro(Nulo{}, true); }
Parametro Parametro::data(QDate) { return Parametro(Nulo{}, false); }

const Parametro::Valor& Parametro::valor() const noexcept { return valor_; }
bool Parametro::ehBytes() const noexcept { return bytes_; }

Parametro::Parametro(Valor valor, bool bytes) : valor_(std::move(valor)), bytes_(bytes) {}

} // namespace nl::foundation::sql
