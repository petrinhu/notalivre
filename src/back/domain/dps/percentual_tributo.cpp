// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de PercentualTributo (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1479 (TSDec3V2).

#include <nl/back/dps/percentual_tributo.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<PercentualTributo, nl::base::Erro> PercentualTributo::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD(0|0\.[0-9]{2}|[1-9]{1}[0-9]{0,2}(\.[0-9]{2})?)PAD")) {
        return std::unexpected(nl::base::Erro{"percentual.formato", std::nullopt, "PercentualTributo: texto fora do padrão do XSD"});
    }
    return PercentualTributo{entrada.toString()};
}

QString PercentualTributo::texto() const
{
    return texto_;
}

PercentualTributo::PercentualTributo(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
