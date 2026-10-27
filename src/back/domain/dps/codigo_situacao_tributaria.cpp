// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoSituacaoTributaria (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1810 (TSRTCCodSitTrib).

#include <nl/back/dps/codigo_situacao_tributaria.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoSituacaoTributaria, nl::base::Erro> CodigoSituacaoTributaria::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{3})PAD")) {
        return std::unexpected(nl::base::Erro{"cst.formato", std::nullopt, "CodigoSituacaoTributaria: texto fora do padrão do XSD"});
    }
    return CodigoSituacaoTributaria{entrada.toString()};
}

QString CodigoSituacaoTributaria::texto() const
{
    return texto_;
}

CodigoSituacaoTributaria::CodigoSituacaoTributaria(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
