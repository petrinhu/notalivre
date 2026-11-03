// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Caso de uso EmitirNota (NL-2.9, DA-2.42). Ver emitir_nota.h.
#include <nl/back/app/emitir_nota.h>

#include <optional>

namespace nl::back {

EmitirNota::EmitirNota(AssinadorDps& assinador) noexcept
    : assinador_(assinador) {}

// Invariante antes do port: a assinatura precisa de um Id de referência. Id vazio nunca chega ao assinador.
std::expected<QByteArray, nl::base::Erro> EmitirNota::executar(QByteArrayView dpsXml, QStringView idDps) {
    if (idDps.isEmpty()) {
        return std::unexpected(nl::base::Erro("emissao.id_vazio", std::nullopt, "a DPS não tem Id para assinar"));
    }
    return assinador_.assinar(dpsXml, idDps);
}

} // namespace nl::back
