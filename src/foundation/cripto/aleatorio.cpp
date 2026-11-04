// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Bytes aleatórios do sistema (NL-3.2b) pelo RAND_bytes do OpenSSL.

#include <nl/foundation/cripto/aleatorio.h>

#include "nl/foundation/cripto/erros.h"

#include <openssl/rand.h>

#include <limits>

namespace nl::foundation::cripto {

std::expected<void, nl::base::Erro> bytesAleatorios(std::span<unsigned char> saida)
{
    if (saida.empty()) {
        return {};
    }
    if (saida.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "pedido de bytes aleatórios grande demais"));
    }
    if (RAND_bytes(saida.data(), static_cast<int>(saida.size())) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o gerador aleatório do sistema falhou"));
    }
    return {};
}

} // namespace nl::foundation::cripto
