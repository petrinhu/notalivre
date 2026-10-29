// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Compactação gzip (RFC 1952) da Foundation (NL-2.4, DA-2.4). Motor agnóstico: depende só de base (DA-2.30).
// O contrato é o formato RFC 1952. Não é zlib cru (RFC 1950) nem o prefixo de tamanho do qCompress do Qt.
// A implementação (zlib do sistema ou gzip próprio) ainda não foi escolhida; decisão do líder (L-51).

#ifndef NL_FOUNDATION_GZIP_GZIP_H
#define NL_FOUNDATION_GZIP_GZIP_H

#include <nl/base/erro.h>

#include <cstddef>
#include <expected>
#include <span>
#include <vector>

namespace nl::foundation::gzip {

/// Compacta a entrada no formato gzip (RFC 1952): cabeçalho 1f 8b 08, blocos deflate e rodapé com CRC-32 e ISIZE em
/// little-endian. Entrada vazia é um gzip válido de zero bytes; quem chama decide se aceita isso.
[[nodiscard]] std::expected<std::vector<std::byte>, nl::base::Erro> comprimir(std::span<const std::byte> entrada);

} // namespace nl::foundation::gzip

#endif // NL_FOUNDATION_GZIP_GZIP_H
