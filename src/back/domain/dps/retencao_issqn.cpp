// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de RetencaoIssqn (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1130 (TSTipoRetISSQN).

#include <nl/back/dps/retencao_issqn.h>

#include <optional>

namespace nl::back {

std::expected<RetencaoIssqn, nl::base::Erro> RetencaoIssqn::de(QStringView entrada)
{
    if (entrada == u"1") {
        return RetencaoIssqn{RetencaoIssqn::Valor::naoRetido};
    }
    if (entrada == u"2") {
        return RetencaoIssqn{RetencaoIssqn::Valor::retidoPeloTomador};
    }
    if (entrada == u"3") {
        return RetencaoIssqn{RetencaoIssqn::Valor::retidoPeloIntermediario};
    }
    return std::unexpected(nl::base::Erro{"retencao.valor", std::nullopt, "RetencaoIssqn: valor fora do conjunto do XSD"});
}

} // namespace nl::back
