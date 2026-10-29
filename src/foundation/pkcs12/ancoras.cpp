// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// NL-2.2, fase vermelha: nenhum PEM é lido ainda.
#include <nl/foundation/pkcs12/ancoras.h>

#include <utility>

namespace nl::foundation::pkcs12 {

std::expected<Ancoras, ErroAncora> Ancoras::dePem(QByteArrayView) {
    return std::unexpected(ErroAncora::PemSemCertificado);
}

Ancoras::Ancoras(std::vector<CertificadoX509> certificados) noexcept
    : certificados_(std::move(certificados)) {}

std::size_t Ancoras::quantidade() const noexcept {
    return certificados_.size();
}

std::vector<x509_st*> Ancoras::certificados() const {
    return {};
}

} // namespace nl::foundation::pkcs12
