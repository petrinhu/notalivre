// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de PercentualTributoSn (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1470 (TSDec2V2).

#include <nl/back/dps/percentual_tributo_sn.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<PercentualTributoSn, nl::base::Erro> PercentualTributoSn::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD(0|0\.[0-9]{2}|[1-9]{1}[0-9]{0,1}(\.[0-9]{2})?)PAD")) {
        return std::unexpected(nl::base::Erro{"percentual_sn.formato", std::nullopt, "PercentualTributoSn: texto fora do padrão do XSD"});
    }
    return PercentualTributoSn{entrada.toString()};
}

QString PercentualTributoSn::texto() const
{
    return texto_;
}

PercentualTributoSn::PercentualTributoSn(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
