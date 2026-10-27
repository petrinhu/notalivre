// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Motivo para não informar o NIF (cNaoNIF), alternativa ao identificador do tomador. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:394-402 (TSCodNaoNIF). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_MOTIVO_NAO_NIF_H
#define NL_BACK_DPS_MOTIVO_NAO_NIF_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Motivo para não informar o NIF (cNaoNIF), alternativa ao identificador do tomador.
/// Só se constrói por MotivoNaoNif::de, que aceita somente os valores do XSD. O construtor é privado.
class MotivoNaoNif
{
public:
    enum class Valor
    {
        naoInformadoNaOrigem, // XML "0": 0 - Não informado na nota de origem
        dispensadoDoNif, // XML "1": 1 - Dispensado do NIF
        naoExigenciaDoNif, // XML "2": 2 - Não exigência do NIF
    };

    /// Erro: "nif.motivo" (valor fora do conjunto do XSD).
    static std::expected<MotivoNaoNif, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const MotivoNaoNif& outro) const noexcept = default;

private:
    explicit MotivoNaoNif(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_MOTIVO_NAO_NIF_H
