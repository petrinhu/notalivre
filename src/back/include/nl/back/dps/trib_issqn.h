// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Tipo de tributação do ISSQN (tribISSQN). (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1075 (TSTribISSQN). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_TRIB_ISSQN_H
#define NL_BACK_DPS_TRIB_ISSQN_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Tipo de tributação do ISSQN (tribISSQN).
/// Só se constrói por TribIssqn::de, que aceita somente os valores do XSD. O construtor é privado.
class TribIssqn
{
public:
    enum class Valor
    {
        operacaoTributavel, // XML "1": 1 - Operação tributável
        imunidade, // XML "2": 2 - Imunidade
        exportacaoServico, // XML "3": 3 - Exportação de serviço
        naoIncidencia, // XML "4": 4 - Não incidência
    };

    /// Erro: "tribissqn.valor" (valor fora do conjunto do XSD).
    static std::expected<TribIssqn, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const TribIssqn& outro) const noexcept = default;

private:
    explicit TribIssqn(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TRIB_ISSQN_H
