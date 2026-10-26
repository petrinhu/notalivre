// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Situação do prestador perante o Simples Nacional (opSimpNac): 1, 2 e 3 (DA-1.16). O 4 não existe no XSD de 20/10. (NL-1.4). Fonte: tiposComplexos_v1.01.xsd:968 e tiposSimples_v1.01.xsd:1003-1016 (TSOpSimpNac). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_OP_SIMP_NAC_H
#define NL_BACK_DPS_OP_SIMP_NAC_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Situação do prestador perante o Simples Nacional (opSimpNac): 1, 2 e 3 (DA-1.16). O 4 não existe no XSD de 20/10.
/// Só se constrói por OpSimpNac::de, que aceita somente os valores do XSD. O construtor é privado.
class OpSimpNac
{
public:
    enum class Valor
    {
        naoOptante, // XML "1": 1 - Não optante
        mei, // XML "2": 2 - Optante MEI
        meEpp, // XML "3": 3 - Optante ME/EPP
    };

    /// Erro: "regime.opsimpnac" (valor fora do conjunto do XSD).
    static std::expected<OpSimpNac, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const OpSimpNac& outro) const noexcept = default;

private:
    explicit OpSimpNac(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_OP_SIMP_NAC_H
