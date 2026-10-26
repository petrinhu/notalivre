// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoNbs (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1384 (TSCodNBS; elemento cNBS em tiposComplexos_v1.01.xsd:1483).

#include <nl/back/dps/codigo_nbs.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoNbs, nl::base::Erro> CodigoNbs::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{9})PAD")) {
        return std::unexpected(nl::base::Erro{"cnbs.formato", std::nullopt, "CodigoNbs: texto fora do padrão do XSD"});
    }
    return CodigoNbs{entrada.toString()};
}

QString CodigoNbs::texto() const
{
    return texto_;
}

CodigoNbs::CodigoNbs(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
