// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Texto de tamanho entre Minimo e Maximo caracteres Unicode (NL-1.4, DA-1.2). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_TEXTO_LIMITADO_H
#define NL_BACK_DPS_TEXTO_LIMITADO_H

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

#include <QList>
#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Texto com tamanho entre Minimo e Maximo, contado em pontos de código (não em unidades UTF-16 do QString).
/// Invariante: só se constrói por TextoLimitado::de. O construtor é privado.
template <int Minimo, int Maximo>
class TextoLimitado
{
    static_assert(Minimo >= 0 && Minimo <= Maximo, "faixa de tamanho inválida");

public:
    /// Erro: "texto.tamanho" (fora da faixa), com o campo do leiaute.
    static std::expected<TextoLimitado, nl::base::Erro> de(QStringView entrada, std::string_view campo)
    {
        const qsizetype tamanho = entrada.toString().toUcs4().size();
        if (tamanho < Minimo || tamanho > Maximo) {
            return std::unexpected(nl::base::Erro{
                "texto.tamanho", std::string{campo},
                std::string{"campo "} + std::string{campo} + " deve ter de " + std::to_string(Minimo) + " a " + std::to_string(Maximo) +
                    " caracteres"});
        }
        return TextoLimitado{entrada.toString()};
    }

    /// Texto como veio, já validado.
    QString texto() const { return texto_; }

    bool operator==(const TextoLimitado& outro) const = default;

private:
    explicit TextoLimitado(QString texto) : texto_{std::move(texto)} {}

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TEXTO_LIMITADO_H
