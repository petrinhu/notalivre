// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Forma canônica da senha (NL-3.2b, ADR-0003, entrada canônica: senha em UTF-8 na forma NFC). Motor agnóstico.
// Senha com UTF-8 inválido é recusada, nunca convertida em silêncio: duas senhas diferentes não podem dar a mesma chave.

#ifndef NL_FOUNDATION_CRIPTO_SENHA_NFC_H
#define NL_FOUNDATION_CRIPTO_SENHA_NFC_H

#include <nl/base/erro.h>

#include <QByteArrayView>

#include <expected>
#include <span>

namespace nl::foundation::cripto {

/// Escreve a senha na forma NFC, em UTF-8, na área de destino, e devolve o tamanho escrito.
/// UTF-8 inválido (truncado, forma longa, substituto ou acima de U+10FFFF): cripto.utf8_invalido, sem converter.
/// Destino pequeno: cripto.tamanho.
[[nodiscard]] std::expected<int, nl::base::Erro> senhaEmNfc(QByteArrayView utf8, std::span<unsigned char> destino);

} // namespace nl::foundation::cripto

#endif // NL_FOUNDATION_CRIPTO_SENHA_NFC_H
