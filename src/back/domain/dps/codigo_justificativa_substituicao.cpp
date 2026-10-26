// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoJustificativaSubstituicao (NL-1.4). Fonte: tiposSimples_v1.01.xsd:235-250 (TSCodJustSubst).

#include <nl/back/dps/codigo_justificativa_substituicao.h>

#include <optional>

namespace nl::back {

std::expected<CodigoJustificativaSubstituicao, nl::base::Erro> CodigoJustificativaSubstituicao::de(QStringView entrada)
{
    if (entrada == u"01") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::desenquadramentoSimplesNacional};
    }
    if (entrada == u"02") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::enquadramentoSimplesNacional};
    }
    if (entrada == u"03") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::inclusaoRetroativaImunidade};
    }
    if (entrada == u"04") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::exclusaoRetroativaImunidade};
    }
    if (entrada == u"05") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::rejeicaoPeloTomador};
    }
    if (entrada == u"99") {
        return CodigoJustificativaSubstituicao{CodigoJustificativaSubstituicao::Valor::outros};
    }
    return std::unexpected(nl::base::Erro{"substituicao.motivo", std::nullopt, "CodigoJustificativaSubstituicao: valor fora do conjunto do XSD"});
}

} // namespace nl::back
