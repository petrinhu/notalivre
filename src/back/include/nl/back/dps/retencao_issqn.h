// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tipo de retenção do ISSQN (tpRetISSQN), obrigatório. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1130 (TSTipoRetISSQN). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_RETENCAO_ISSQN_H
#define NL_BACK_DPS_RETENCAO_ISSQN_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Tipo de retenção do ISSQN (tpRetISSQN), obrigatório.
/// Só se constrói por RetencaoIssqn::de, que aceita somente os valores do XSD. O construtor é privado.
class RetencaoIssqn
{
public:
    enum class Valor
    {
        naoRetido, // XML "1": 1 - Não retido
        retidoPeloTomador, // XML "2": 2 - Retido pelo tomador
        retidoPeloIntermediario, // XML "3": 3 - Retido pelo intermediário
    };

    /// Erro: "retencao.valor" (valor fora do conjunto do XSD).
    static std::expected<RetencaoIssqn, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const RetencaoIssqn& outro) const noexcept = default;

private:
    explicit RetencaoIssqn(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_RETENCAO_ISSQN_H
