// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador AssinadorDpsSefin (NL-2.9, DA-2.42). Ver assinador_dps_sefin.h.
// FATIA VERMELHA: o assinar ainda não assina. A implementação vem na fatia verde.
#include <nl/mid/sefin/assinatura/assinador_dps_sefin.h>

#include <optional>

namespace nl::mid::sefin::assinatura {

AssinadorDpsSefin::AssinadorDpsSefin(const nl::foundation::pkcs12::CredencialX509& credencial) noexcept
    : credencial_(credencial) {}

std::expected<QByteArray, nl::base::Erro> AssinadorDpsSefin::assinar(QByteArrayView xml, QStringView idReferencia) {
    static_cast<void>(xml);
    static_cast<void>(idReferencia);
    return std::unexpected(nl::base::Erro("assinatura.nao_implementada", std::nullopt, "assinador ainda não implementado"));
}

} // namespace nl::mid::sefin::assinatura
