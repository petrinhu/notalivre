// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação de Telefone (NL-1.4). Padrão: tiposSimples_v1.01.xsd:558 (TSTelefone).

#include <nl/back/dps/telefone.h>

#include <nl/back/dps/casa_padrao.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<Telefone, nl::base::Erro> Telefone::de(QStringView entrada)
{
    if (!detalhe::casaPadrao(entrada, R"PAD([0-9]{6,20})PAD")) {
        return std::unexpected(nl::base::Erro{"telefone.formato", std::nullopt, "Telefone: texto fora do padrão do XSD"});
    }
    return Telefone{entrada.toString()};
}

QString Telefone::texto() const
{
    return texto_;
}

Telefone::Telefone(QString texto)
    : texto_{std::move(texto)}
{
}

} // namespace nl::back
