// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação da interface RegraAviso (NL-1.5): só o destrutor virtual, que ancora o vtable.

#include <nl/back/regras/regra_aviso.h>

namespace nl::back {

RegraAviso::~RegraAviso() = default;

} // namespace nl::back
