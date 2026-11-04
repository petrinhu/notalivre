// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Costura interna da senhaEmNfc (NL-3.2b). Privada ao motor: só o próprio .cpp e os testes da cripto a incluem.
// O relatório é por chamada, sem contador global nem variável de estado compartilhada (sem singleton mutável).

#ifndef NL_FOUNDATION_CRIPTO_PRIVADO_SENHA_NFC_INTERNO_H
#define NL_FOUNDATION_CRIPTO_PRIVADO_SENHA_NFC_INTERNO_H

#include <nl/base/erro.h>

#include <QByteArrayView>

#include <expected>
#include <span>

namespace nl::foundation::cripto::detalhe {

/// O que a chamada fez. passouPorQString vira verdadeiro só quando a conversão usou QString (byte não ASCII).
struct RelatorioNfc {
    bool passouPorQString = false;
};

/// Mesma função que senhaEmNfc, com o relatório da chamada. Entrada ASCII nunca passa por QString (ADR-0003).
[[nodiscard]] std::expected<int, nl::base::Erro> senhaEmNfcComRelatorio(QByteArrayView utf8,
                                                                         std::span<unsigned char> destino,
                                                                         RelatorioNfc& relatorio);

} // namespace nl::foundation::cripto::detalhe

#endif // NL_FOUNDATION_CRIPTO_PRIVADO_SENHA_NFC_INTERNO_H
