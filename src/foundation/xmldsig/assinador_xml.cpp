// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.3): a assinatura real entra no commit verde, depois dos testes.
#include <nl/foundation/xmldsig/assinador_xml.h>

namespace nl::foundation::xmldsig {

std::string_view codigo(ErroAssinatura) noexcept {
    return {};
}

std::expected<QByteArray, ErroAssinatura> AssinadorXml::assinarEnveloped(
    QByteArrayView, QStringView, const ParametrosAssinatura&, const MaterialAssinatura&) {
    return std::unexpected(ErroAssinatura::FalhaInterna);
}

} // namespace nl::foundation::xmldsig
