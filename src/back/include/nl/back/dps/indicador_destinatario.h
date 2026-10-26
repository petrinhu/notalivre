// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// A respeito do destinatário dos serviços (indDest), obrigatório quando o grupo IBS/CBS existe. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1758-1768 (TSRTCIndDest). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_INDICADOR_DESTINATARIO_H
#define NL_BACK_DPS_INDICADOR_DESTINATARIO_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// A respeito do destinatário dos serviços (indDest), obrigatório quando o grupo IBS/CBS existe.
/// Só se constrói por IndicadorDestinatario::de, que aceita somente os valores do XSD. O construtor é privado.
class IndicadorDestinatario
{
public:
    enum class Valor
    {
        tomadorEDestinatario, // XML "0": 0 - Destinatário é o próprio tomador
        outroDestinatario, // XML "1": 1 - Destinatário é outra pessoa
    };

    /// Erro: "indicadordestinatario.valor" (valor fora do conjunto do XSD).
    static std::expected<IndicadorDestinatario, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const IndicadorDestinatario& outro) const noexcept = default;

private:
    explicit IndicadorDestinatario(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_INDICADOR_DESTINATARIO_H
