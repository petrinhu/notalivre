// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Indicador do total aproximado de tributos (indTotTrib): único valor 0. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1268 (TSTipoIndTotTrib). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_INDICADOR_TOTAL_TRIBUTOS_H
#define NL_BACK_DPS_INDICADOR_TOTAL_TRIBUTOS_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Indicador do total aproximado de tributos (indTotTrib): único valor 0.
/// Só se constrói por IndicadorTotalTributos::de, que aceita somente os valores do XSD. O construtor é privado.
class IndicadorTotalTributos
{
public:
    enum class Valor
    {
        naoInformar, // XML "0": 0 - Não informar
    };

    /// Erro: "indtottrib.valor" (valor fora do conjunto do XSD).
    static std::expected<IndicadorTotalTributos, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const IndicadorTotalTributos& outro) const noexcept = default;

private:
    explicit IndicadorTotalTributos(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_INDICADOR_TOTAL_TRIBUTOS_H
