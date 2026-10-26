// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de TipoOperacao (NL-1.4). Fonte: tiposSimples_v1.01.xsd:1719-1736 (TSRTCTpOper).

#include <nl/back/dps/tipo_operacao.h>

#include <optional>

namespace nl::back {

std::expected<TipoOperacao, nl::base::Erro> TipoOperacao::de(QStringView entrada)
{
    if (entrada == u"1") {
        return TipoOperacao{TipoOperacao::Valor::fornecimentoComPagamentoPosterior};
    }
    if (entrada == u"2") {
        return TipoOperacao{TipoOperacao::Valor::recebimentoComFornecimentoRealizado};
    }
    if (entrada == u"3") {
        return TipoOperacao{TipoOperacao::Valor::fornecimentoComPagamentoRealizado};
    }
    if (entrada == u"4") {
        return TipoOperacao{TipoOperacao::Valor::recebimentoComFornecimentoPosterior};
    }
    if (entrada == u"5") {
        return TipoOperacao{TipoOperacao::Valor::fornecimentoERecebimentoConcomitantes};
    }
    return std::unexpected(nl::base::Erro{"tipooperacao.valor", std::nullopt, "TipoOperacao: valor fora do conjunto do XSD"});
}

} // namespace nl::back
