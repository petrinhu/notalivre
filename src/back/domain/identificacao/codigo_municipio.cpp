// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do código de município (NL-1.1): exatamente 7 dígitos, texto preservado.

#include <nl/back/identificacao/codigo_municipio.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr qsizetype kTamanho = 7;

constexpr bool digito(char16_t c) noexcept
{
    return c >= u'0' && c <= u'9';
}

} // namespace

std::expected<CodigoMunicipio, nl::base::Erro> CodigoMunicipio::de(QStringView entrada)
{
    for (const QChar c : entrada) {
        if (!digito(c.unicode())) {
            return std::unexpected(nl::base::Erro{"municipio.formato", std::nullopt,
                                                  "código de município aceita só dígitos"});
        }
    }
    if (entrada.size() != kTamanho) {
        return std::unexpected(nl::base::Erro{"municipio.tamanho", std::nullopt,
                                              "código de município deve ter 7 dígitos, sem completar zero"});
    }
    return CodigoMunicipio{QString{entrada}};
}

QString CodigoMunicipio::texto() const
{
    return texto_;
}

CodigoMunicipio::CodigoMunicipio(QString texto) : texto_{std::move(texto)}
{
}

} // namespace nl::back
