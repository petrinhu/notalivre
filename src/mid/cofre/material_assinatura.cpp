// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conversão CredencialX509 -> MaterialAssinatura (NL-2.3, DA-2.36): só empresta ponteiros, na chamada síncrona.
#include <nl/mid/cofre/material_assinatura.h>

#include <nl/foundation/pkcs12/credencial_x509.h>

namespace nl::mid::cofre {

std::expected<nl::foundation::xmldsig::MaterialAssinatura, ErroMaterial>
materialParaAssinatura(const nl::foundation::pkcs12::CredencialX509& credencial) noexcept {
    if (credencial.descartada()) {
        return std::unexpected(ErroMaterial::CredencialDescartada);
    }
    return nl::foundation::xmldsig::MaterialAssinatura{credencial.certificado(), credencial.chavePrivada()};
}

} // namespace nl::mid::cofre
