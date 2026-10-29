// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.3, DA-2.36): a conversão real entra no commit verde, depois dos testes.
#include <nl/mid/cofre/material_assinatura.h>

namespace nl::mid::cofre {

std::expected<nl::foundation::xmldsig::MaterialAssinatura, ErroMaterial>
materialParaAssinatura(const nl::foundation::pkcs12::CredencialX509&) noexcept {
    return std::unexpected(ErroMaterial::CredencialDescartada);
}

} // namespace nl::mid::cofre
