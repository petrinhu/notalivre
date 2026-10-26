// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Identificação do ambiente da DPS (tpAmb). Produção Restrita é emitida com o valor 2 (spec 4.12). (NL-1.4). Fonte: tiposSimples_v1.01.xsd:57-66 (TSTipoAmbiente). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_TIPO_AMBIENTE_H
#define NL_BACK_DPS_TIPO_AMBIENTE_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Identificação do ambiente da DPS (tpAmb). Produção Restrita é emitida com o valor 2 (spec 4.12).
/// Só se constrói por TipoAmbiente::de, que aceita somente os valores do XSD. O construtor é privado.
class TipoAmbiente
{
public:
    enum class Valor
    {
        producao, // XML "1": 1 - Produção
        homologacao, // XML "2": 2 - Homologação
    };

    /// Erro: "ambiente.valor" (valor fora do conjunto do XSD).
    static std::expected<TipoAmbiente, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const TipoAmbiente& outro) const noexcept = default;

private:
    explicit TipoAmbiente(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TIPO_AMBIENTE_H
