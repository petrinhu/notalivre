// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do port CatalogoLocalidades (NL-1.5): só o destrutor virtual, que ancora o vtable.

#include <nl/back/ports/catalogo_localidades.h>

namespace nl::back {

CatalogoLocalidades::~CatalogoLocalidades() = default;

} // namespace nl::back
