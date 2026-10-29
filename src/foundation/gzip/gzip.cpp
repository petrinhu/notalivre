// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// STUB VERMELHO (NL-2.4, commit de teste): ainda não compacta. Os testes de tst_gzip têm de falhar aqui.

#include <nl/foundation/gzip/gzip.h>

#include <nl/base/erro.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <span>
#include <vector>

namespace nl::foundation::gzip {

std::expected<std::vector<std::byte>, nl::base::Erro> comprimir(std::span<const std::byte>)
{
    return std::unexpected(nl::base::Erro{"gzip.nao_implementado", std::nullopt, "gzip ainda não implementado"});
}

} // namespace nl::foundation::gzip
