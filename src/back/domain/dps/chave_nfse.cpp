// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de ChaveNfse (NL-1.4). Padrão: tiposSimples_v1.01.xsd:206 (TSChaveNFSe).

#include <nl/back/dps/chave_nfse.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<ChaveNfse, nl::base::Erro> ChaveNfse::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{6}([0-9A-Z]{14})[0-9]{30})PAD")) {
        return std::unexpected(nl::base::Erro{"chave_nfse.formato", std::nullopt, "ChaveNfse: texto fora do padrão do XSD"});
    }
    return ChaveNfse{entrada.toString()};
}

QString ChaveNfse::texto() const
{
    return texto_;
}

ChaveNfse::ChaveNfse(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
