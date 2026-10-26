// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do CEP (NL-1.1, TSCEP): remove hífen e espaço, exige 8 dígitos.

#include <nl/back/identificacao/cep.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr qsizetype kTamanho = 8;

constexpr bool separadorPermitido(char16_t c) noexcept
{
    return c == u'-' || c == u' ';
}

constexpr bool digito(char16_t c) noexcept
{
    return c >= u'0' && c <= u'9';
}

} // namespace

std::expected<Cep, nl::base::Erro> Cep::de(QStringView entrada)
{
    QString digitos;
    digitos.reserve(entrada.size());
    for (const QChar c : entrada) {
        if (!separadorPermitido(c.unicode())) {
            digitos.append(c);
        }
    }

    for (const QChar c : digitos) {
        if (!digito(c.unicode())) {
            return std::unexpected(nl::base::Erro{"cep.formato", std::nullopt, "CEP aceita só dígitos, com hífen ou espaço opcionais"});
        }
    }
    if (digitos.size() != kTamanho) {
        return std::unexpected(nl::base::Erro{"cep.tamanho", std::nullopt,
                                              "CEP deve ter 8 dígitos, sem completar zero"});
    }
    return Cep{digitos};
}

QString Cep::texto() const
{
    return texto_;
}

Cep::Cep(QString texto) : texto_{std::move(texto)}
{
}

} // namespace nl::back
