// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de AliquotaIssqn (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1461 (TSDec1V2; elemento pAliq em tiposComplexos_v1.01.xsd:2034).

#include <nl/back/dps/aliquota_issqn.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<AliquotaIssqn, nl::base::Erro> AliquotaIssqn::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD(0|[0-9]{1}(\.[0-9]{2})?)PAD")) {
        return std::unexpected(nl::base::Erro{"aliquota_issqn.formato", std::nullopt, "AliquotaIssqn: texto fora do padrão do XSD"});
    }
    return AliquotaIssqn{entrada.toString()};
}

QString AliquotaIssqn::texto() const
{
    return texto_;
}

AliquotaIssqn::AliquotaIssqn(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
