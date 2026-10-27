// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do port RepositorioNumeracao (NL-1.3). Só o destrutor virtual: ancora o vtable da interface.

#include <nl/back/ports/repositorio_numeracao.h>

namespace nl::back {

RepositorioNumeracao::~RepositorioNumeracao() = default;

} // namespace nl::back
