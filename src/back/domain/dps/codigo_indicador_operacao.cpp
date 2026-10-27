// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoIndicadorOperacao (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1716 (TSRTCCodIndOp).

#include <nl/back/dps/codigo_indicador_operacao.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoIndicadorOperacao, nl::base::Erro> CodigoIndicadorOperacao::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{6})PAD")) {
        return std::unexpected(nl::base::Erro{"cindop.formato", std::nullopt, "CodigoIndicadorOperacao: texto fora do padrão do XSD"});
    }
    return CodigoIndicadorOperacao{entrada.toString()};
}

QString CodigoIndicadorOperacao::texto() const
{
    return texto_;
}

CodigoIndicadorOperacao::CodigoIndicadorOperacao(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
