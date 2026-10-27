// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de CodigoClassificacaoTributaria (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1821 (TSRTCCodClassTrib).

#include <nl/back/dps/codigo_classificacao_tributaria.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<CodigoClassificacaoTributaria, nl::base::Erro> CodigoClassificacaoTributaria::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{6})PAD")) {
        return std::unexpected(nl::base::Erro{"cclasstrib.formato", std::nullopt, "CodigoClassificacaoTributaria: texto fora do padrão do XSD"});
    }
    return CodigoClassificacaoTributaria{entrada.toString()};
}

QString CodigoClassificacaoTributaria::texto() const
{
    return texto_;
}

CodigoClassificacaoTributaria::CodigoClassificacaoTributaria(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
