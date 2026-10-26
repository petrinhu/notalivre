// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Número da DPS (NL-1.3): inteiro de 1 a 999999999999999 escrito com quinze posições (TSIdDPS, tiposSimples_v1.01.xsd:54). Regra pura.

#ifndef NL_BACK_DPS_NUMERO_DPS_H
#define NL_BACK_DPS_NUMERO_DPS_H

#include <cstdint>
#include <expected>

#include <QString>

#include <nl/base/erro.h>

namespace nl::back {

/// Número da DPS. Invariante: valor entre 1 e 999999999999999. Só se constrói por NumeroDps::de. O construtor é privado.
class NumeroDps
{
public:
    /// Erro: "numero.faixa" (zero, negativo ou acima de quinze dígitos).
    static std::expected<NumeroDps, nl::base::Erro> de(std::int64_t valor);

    /// Valor numérico, já validado.
    std::int64_t valor() const noexcept { return valor_; }

    /// As quinze posições, com zero à esquerda (por exemplo "000000000000001").
    QString texto() const;

    bool operator==(const NumeroDps& outro) const = default;

private:
    explicit NumeroDps(std::int64_t valor);

    std::int64_t valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_NUMERO_DPS_H
