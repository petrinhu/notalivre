// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do CNPJ (NL-1.1, DA-1.1): normalização, alfabeto, tamanho, posições 13-14 e DV módulo 11.

#include <nl/back/identificacao/cnpj.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr qsizetype kTamanho = 14;
constexpr qsizetype kTamanhoBase = 12;
constexpr qsizetype kTamanhoRaiz = 8;

// DA-1.1: só estes quatro separadores saem; tabulação e os demais ficam e viram erro de formato.
constexpr bool separadorPermitido(char16_t c) noexcept
{
    return c == u'.' || c == u'/' || c == u'-' || c == u' ';
}

// DA-1.1: conversão só ASCII. Unicode (por exemplo U+0131) não pode virar letra do alfabeto por caixa alta.
constexpr char16_t maiusculaAscii(char16_t c) noexcept
{
    return (c >= u'a' && c <= u'z') ? static_cast<char16_t>(c - (u'a' - u'A')) : c;
}

constexpr bool caractereDoAlfabeto(char16_t c) noexcept
{
    return (c >= u'0' && c <= u'9') || (c >= u'A' && c <= u'Z');
}

constexpr bool digito(char16_t c) noexcept
{
    return c >= u'0' && c <= u'9';
}

// Valor para o cálculo: código ASCII menos 48 (Serpro 1.1; RFB Q.14). Dígito vale o próprio dígito.
constexpr int valorParaCalculo(char16_t c) noexcept
{
    return static_cast<int>(c) - 48;
}

// Módulo 11 com pesos de 2 a 9 da direita para a esquerda, recomeçando após o oitavo. Resto 0 ou 1 dá DV 0.
int digitoVerificador(QStringView base) noexcept
{
    const qsizetype n = base.size();
    int soma = 0;
    for (qsizetype i = 0; i < n; ++i) {
        const int peso = 2 + static_cast<int>((n - 1 - i) % 8);
        soma += valorParaCalculo(base[i].unicode()) * peso;
    }
    const int resto = soma % 11;
    return resto < 2 ? 0 : 11 - resto;
}

} // namespace

QString Cnpj::normalizar(QStringView entrada)
{
    QString normalizado;
    normalizado.reserve(entrada.size());
    for (const QChar c : entrada) {
        if (separadorPermitido(c.unicode())) {
            continue;
        }
        normalizado.append(QChar{maiusculaAscii(c.unicode())});
    }
    return normalizado;
}

std::expected<Cnpj, nl::base::Erro> Cnpj::de(QStringView entrada)
{
    const QString normalizado = normalizar(entrada);

    for (const QChar c : normalizado) {
        if (!caractereDoAlfabeto(c.unicode())) {
            return std::unexpected(nl::base::Erro{"cnpj.formato", std::nullopt,
                                                  "CNPJ aceita só dígitos e letras A a Z, sem acento"});
        }
    }
    if (normalizado.size() != kTamanho) {
        return std::unexpected(nl::base::Erro{"cnpj.tamanho", std::nullopt,
                                              "CNPJ deve ter 14 caracteres, sem contar a máscara"});
    }
    if (!digito(normalizado.at(12).unicode()) || !digito(normalizado.at(13).unicode())) {
        return std::unexpected(nl::base::Erro{"cnpj.formato", std::nullopt,
                                              "as posições 13 e 14 do CNPJ são dígitos verificadores"});
    }

    // DA-1.9 (decisão do CTO): só o valor todo zero é rejeitado; o DV dele confere (soma zero), então a regra é explícita.
    if (normalizado == QStringLiteral("00000000000000")) {
        return std::unexpected(nl::base::Erro{"cnpj.zerado", std::nullopt,
                                              "CNPJ com todos os dígitos zero não é válido"});
    }

    const QStringView vista{normalizado};
    const int dv1 = digitoVerificador(vista.first(kTamanhoBase));
    const int dv2 = digitoVerificador(vista.first(kTamanhoBase + 1));
    if (valorParaCalculo(normalizado.at(12).unicode()) != dv1 ||
        valorParaCalculo(normalizado.at(13).unicode()) != dv2) {
        return std::unexpected(nl::base::Erro{"cnpj.dv", std::nullopt,
                                              "dígito verificador do CNPJ não confere"});
    }
    return Cnpj{normalizado};
}

QString Cnpj::texto() const
{
    return texto_;
}

QString Cnpj::raiz() const
{
    return texto_.left(kTamanhoRaiz);
}

QString Cnpj::comMascara() const
{
    return QStringLiteral("%1.%2.%3/%4-%5")
        .arg(texto_.mid(0, 2), texto_.mid(2, 3), texto_.mid(5, 3), texto_.mid(8, 4), texto_.mid(12, 2));
}

Cnpj::Cnpj(QString normalizado) : texto_{std::move(normalizado)}
{
}

} // namespace nl::back
