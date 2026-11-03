// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conversão da CredencialX509 (NL-2.2) para o MaterialAssinatura do assinador (NL-2.3, DA-2.36). É do adaptador do Mid,
// e vale só durante a chamada síncrona: a credencial tem de viver enquanto o material é usado. Não copia chave, não
// incrementa contagem de referência e não libera nada (teste sob ASan). Cabeçalho público do Mid: só declarações antecipadas.
#ifndef NL_MID_COFRE_MATERIAL_ASSINATURA_H
#define NL_MID_COFRE_MATERIAL_ASSINATURA_H

#include <expected>

namespace nl::foundation::pkcs12 {
class CredencialX509;
}

namespace nl::foundation::xmldsig {
struct MaterialAssinatura;
}

namespace nl::mid::cofre {

enum class ErroMaterial {
    CredencialDescartada,
};

/// Empresta o certificado e a chave da credencial. Credencial descartada devolve erro próprio, nunca ponteiro nulo.
[[nodiscard]] std::expected<nl::foundation::xmldsig::MaterialAssinatura, ErroMaterial>
materialParaAssinatura(const nl::foundation::pkcs12::CredencialX509& credencial) noexcept;

} // namespace nl::mid::cofre

#endif // NL_MID_COFRE_MATERIAL_ASSINATURA_H
