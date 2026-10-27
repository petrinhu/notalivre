// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação da série da DPS (NL-1.3): faixa de 1 a 49999 e cinco posições com zero à esquerda (TSIdDPS, xsd:54).

#include <nl/back/dps/serie.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr int kMinimo = 1;
constexpr int kMaximo = 49999;
constexpr qsizetype kPosicoes = 5;

} // namespace

std::expected<Serie, nl::base::Erro> Serie::de(int valor)
{
    if (valor < kMinimo || valor > kMaximo) {
        return std::unexpected(nl::base::Erro{"serie.faixa", std::nullopt, "série da DPS fora da faixa 1 a 49999"});
    }
    return Serie{valor};
}

Serie::Serie(int valor)
    : valor_{valor}
{
}

QString Serie::texto() const
{
    return QString::number(valor_).rightJustified(kPosicoes, QLatin1Char('0'));
}

} // namespace nl::back
