// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regime especial de tributação do prestador (regEspTrib), obrigatório no XSD. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1049-1062 (TSRegEspTrib). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_REG_ESP_TRIB_H
#define NL_BACK_DPS_REG_ESP_TRIB_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Regime especial de tributação do prestador (regEspTrib), obrigatório no XSD.
/// Só se constrói por RegEspTrib::de, que aceita somente os valores do XSD. O construtor é privado.
class RegEspTrib
{
public:
    enum class Valor
    {
        nenhum, // XML "0": 0 - Nenhum
        atoCooperado, // XML "1": 1 - Ato cooperado
        estimativa, // XML "2": 2 - Estimativa
        microempresaMunicipal, // XML "3": 3 - Microempresa municipal
        notarioRegistrador, // XML "4": 4 - Notário ou registrador
        profissionalAutonomo, // XML "5": 5 - Profissional autônomo
        sociedadeProfissionais, // XML "6": 6 - Sociedade de profissionais
        outros, // XML "9": 9 - Outros
    };

    /// Erro: "regime.regesptrib" (valor fora do conjunto do XSD).
    static std::expected<RegEspTrib, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const RegEspTrib& outro) const noexcept = default;

private:
    explicit RegEspTrib(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_REG_ESP_TRIB_H
