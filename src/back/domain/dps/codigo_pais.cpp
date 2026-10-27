// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoPais (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1362 (TSCodPaisISO).

#include <nl/back/dps/codigo_pais.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoPais, nl::base::Erro> CodigoPais::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([A-Z]{2})PAD")) {
        return std::unexpected(nl::base::Erro{"pais.formato", std::nullopt, "CodigoPais: texto fora do padrão do XSD"});
    }
    return CodigoPais{entrada.toString()};
}

QString CodigoPais::texto() const
{
    return texto_;
}

CodigoPais::CodigoPais(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
