// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Bytes aleatórios do sistema (NL-3.2b). Único sorteio da Foundation de cripto: nonce, salt e chave vêm daqui.

#ifndef NL_FOUNDATION_CRIPTO_ALEATORIO_H
#define NL_FOUNDATION_CRIPTO_ALEATORIO_H

#include <nl/base/erro.h>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// Preenche a área com bytes do gerador seguro do sistema. Falha do gerador: cripto.biblioteca.
[[nodiscard]] std::expected<void, nl::base::Erro> bytesAleatorios(std::span<unsigned char> saida);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_ALEATORIO_H
