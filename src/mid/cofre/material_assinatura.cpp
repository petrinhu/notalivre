// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.3, DA-2.36): recusa tudo, como no commit 4ed933a. A conversão real volta no commit seguinte.
#include <nl/mid/cofre/material_assinatura.h>
#include <nl/foundation/xmldsig/assinador_xml.h>

namespace nl::mid::cofre {

std::expected<nl::foundation::xmldsig::MaterialAssinatura, ErroMaterial>
materialParaAssinatura(const nl::foundation::pkcs12::CredencialX509&) noexcept {
    return std::unexpected(ErroMaterial::CredencialDescartada);
}

} // namespace nl::mid::cofre
