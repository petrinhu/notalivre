// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoTribNacional (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1373 (TSCodTribNac).

#include <nl/back/dps/codigo_trib_nacional.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoTribNacional, nl::base::Erro> CodigoTribNacional::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{6})PAD")) {
        return std::unexpected(nl::base::Erro{"cod_trib_nacional.formato", std::nullopt, "CodigoTribNacional: texto fora do padrão do XSD"});
    }
    return CodigoTribNacional{entrada.toString()};
}

QString CodigoTribNacional::texto() const
{
    return texto_;
}

CodigoTribNacional::CodigoTribNacional(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
