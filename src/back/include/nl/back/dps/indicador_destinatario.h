// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// A respeito do destinatário dos serviços (indDest), obrigatório em toda DPS, na raiz de infDPS (NL-1.4; posição do leiaute de
// 24/11/2026, NL-3.L). Fonte: tiposSimples_v1.01.xsd:1793-1806 (TSRTCIndDest). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_INDICADOR_DESTINATARIO_H
#define NL_BACK_DPS_INDICADOR_DESTINATARIO_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// A respeito do destinatário dos serviços (indDest), obrigatório em toda DPS. O valor 1 exige o grupo dest (E0910),
/// que o app ainda não tem: a regra local E0910 recusa 1 (NL-3.L). O tipo aceita os dois valores do XSD.
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
