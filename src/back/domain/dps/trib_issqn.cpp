// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de TribIssqn (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1075 (TSTribISSQN).

#include <nl/back/dps/trib_issqn.h>

#include <optional>

namespace nl::back {

std::expected<TribIssqn, nl::base::Erro> TribIssqn::de(QStringView entrada)
{
    if (entrada == u"1") {
        return TribIssqn{TribIssqn::Valor::operacaoTributavel};
    }
    if (entrada == u"2") {
        return TribIssqn{TribIssqn::Valor::imunidade};
    }
    if (entrada == u"3") {
        return TribIssqn{TribIssqn::Valor::exportacaoServico};
    }
    if (entrada == u"4") {
        return TribIssqn{TribIssqn::Valor::naoIncidencia};
    }
    return std::unexpected(nl::base::Erro{"tribissqn.valor", std::nullopt, "TribIssqn: valor fora do conjunto do XSD"});
}

} // namespace nl::back
