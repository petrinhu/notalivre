// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de justificativa da substituição (cMotivo), exatamente duas posições. (NL-1.4). Fonte: tiposSimples_v1.01.xsd:235-250 (TSCodJustSubst). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_JUSTIFICATIVA_SUBSTITUICAO_H
#define NL_BACK_DPS_CODIGO_JUSTIFICATIVA_SUBSTITUICAO_H

#include <expected>

#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código de justificativa da substituição (cMotivo), exatamente duas posições.
/// Só se constrói por CodigoJustificativaSubstituicao::de, que aceita somente os valores do XSD. O construtor é privado.
class CodigoJustificativaSubstituicao
{
public:
    enum class Valor
    {
        desenquadramentoSimplesNacional, // XML "01": 01 - Desenquadramento do Simples Nacional
        enquadramentoSimplesNacional, // XML "02": 02 - Enquadramento no Simples Nacional
        inclusaoRetroativaImunidade, // XML "03": 03 - Inclusão retroativa de imunidade ou isenção
        exclusaoRetroativaImunidade, // XML "04": 04 - Exclusão retroativa de imunidade ou isenção
        rejeicaoPeloTomador, // XML "05": 05 - Rejeição pelo tomador ou intermediário
        outros, // XML "99": 99 - Outros
    };

    /// Erro: "substituicao.motivo" (valor fora do conjunto do XSD).
    static std::expected<CodigoJustificativaSubstituicao, nl::base::Erro> de(QStringView entrada);

    Valor valor() const noexcept { return valor_; }

    bool operator==(const CodigoJustificativaSubstituicao& outro) const noexcept = default;

private:
    explicit CodigoJustificativaSubstituicao(Valor valor) noexcept : valor_{valor} {}

    Valor valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_JUSTIFICATIVA_SUBSTITUICAO_H
