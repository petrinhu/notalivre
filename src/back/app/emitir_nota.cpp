// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de uso EmitirNota (NL-2.9, DA-2.42). FATIA VERMELHA: executar ainda não delega ao port.
#include <nl/back/app/emitir_nota.h>

#include <optional>

namespace nl::back {

EmitirNota::EmitirNota(AssinadorDps& assinador) noexcept
    : assinador_(assinador) {}

std::expected<QByteArray, nl::base::Erro> EmitirNota::executar(QByteArrayView dpsXml, QStringView idDps) {
    static_cast<void>(dpsXml);
    static_cast<void>(idDps);
    return std::unexpected(nl::base::Erro("emissao.nao_implementada", std::nullopt, "caso de uso ainda não implementado"));
}

} // namespace nl::back
