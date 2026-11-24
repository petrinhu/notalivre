// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Texto de tamanho entre Minimo e Maximo caracteres Unicode (NL-1.4, DA-1.2), e, nos campos que derivam de TSString, o
// alfabeto do leiaute (NL-3.B1). Regra pura: sem I/O.

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

/// Alfabeto do campo no leiaute. `livre`: xs:string sem pattern, só o tamanho conta. `tsString`: o pattern do TSString
/// (tiposSimples_v1.01.xsd:13-20), `[!-ÿ]{1}[ -ÿ]{0,}[!-ÿ]{1}|[!-ÿ]{1}`: todo caractere de U+0020 a U+00FF, e as duas
/// pontas de U+0021 a U+00FF, isto é, sem espaço U+0020 no começo nem no fim (NL-3.B1).
enum class Alfabeto { livre, tsString };

/// Texto com tamanho entre Minimo e Maximo, contado em pontos de código (não em unidades UTF-16 do QString).
/// Invariante: só se constrói por TextoLimitado::de. O construtor é privado. O texto aceito fica como veio: nada é
/// trocado nem aparado.
template <int Minimo, int Maximo, Alfabeto alfabeto = Alfabeto::livre>
class TextoLimitado
{
    static_assert(Minimo >= 0 && Minimo <= Maximo, "faixa de tamanho inválida");

public:
    /// Erros, com o campo do leiaute, nesta ordem de conferência: "texto.tamanho" (fora da faixa); só no alfabeto
    /// tsString, "texto.caractere" (caractere fora de U+0020 a U+00FF) e "texto.espaco_na_ponta" (espaço no começo ou
    /// no fim).
    static std::expected<TextoLimitado, nl::base::Erro> de(QStringView entrada, std::string_view campo)
    {
        const auto pontos = entrada.toString().toUcs4();   // um item por ponto de código
        const qsizetype tamanho = pontos.size();
        if (tamanho < Minimo || tamanho > Maximo) {
            return std::unexpected(nl::base::Erro{
                "texto.tamanho", std::string{campo},
                std::string{"campo "} + std::string{campo} + " deve ter de " + std::to_string(Minimo) + " a " + std::to_string(Maximo) +
                    " caracteres"});
        }
        // Esboço da NL-3.B1 (passo R): o alfabeto do TSString ainda não é conferido.
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
