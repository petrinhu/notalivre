// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do número da DPS (NL-1.3): faixa de 1 a 999999999999999 e quinze posições (TSIdDPS, xsd:54).

#include <nl/back/dps/numero_dps.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr std::int64_t kMinimo = 1;
constexpr std::int64_t kMaximo = 999999999999999;
constexpr qsizetype kPosicoes = 15;

} // namespace

std::expected<NumeroDps, nl::base::Erro> NumeroDps::de(std::int64_t valor)
{
    if (valor < kMinimo || valor > kMaximo) {
        return std::unexpected(nl::base::Erro{"numero.faixa", std::nullopt, "número da DPS fora da faixa de 1 a quinze dígitos"});
    }
    return NumeroDps{valor};
}

NumeroDps::NumeroDps(std::int64_t valor)
    : valor_{valor}
{
}

QString NumeroDps::texto() const
{
    return QString::number(static_cast<qlonglong>(valor_)).rightJustified(kPosicoes, QLatin1Char('0'));
}

} // namespace nl::back
