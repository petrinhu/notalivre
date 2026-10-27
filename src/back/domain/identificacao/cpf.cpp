// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do CPF (NL-1.1, DA-1.1): remoção de separadores, dígitos, tamanho, repetidos e DV módulo 11.

#include <nl/back/identificacao/cpf.h>

#include <optional>
#include <utility>

namespace nl::back {
namespace {

constexpr qsizetype kTamanho = 11;

// DA-1.1: CPF remove ponto, hífen e espaço. A barra não é separador de CPF: vira erro de formato.
constexpr bool separadorPermitido(char16_t c) noexcept
{
    return c == u'.' || c == u'-' || c == u' ';
}

constexpr bool digito(char16_t c) noexcept
{
    return c >= u'0' && c <= u'9';
}

// Módulo 11 com pesos decrescentes a partir de n + 1 (10 a 2 no primeiro DV, 11 a 2 no segundo). Resto 0 ou 1 dá DV 0.
int digitoVerificador(QStringView base) noexcept
{
    const qsizetype n = base.size();
    int soma = 0;
    for (qsizetype i = 0; i < n; ++i) {
        soma += (static_cast<int>(base[i].unicode()) - 48) * static_cast<int>(n + 1 - i);
    }
    const int resto = soma % 11;
    return resto < 2 ? 0 : 11 - resto;
}

bool todosIguais(QStringView digitos) noexcept
{
    for (qsizetype i = 1; i < digitos.size(); ++i) {
        if (digitos[i] != digitos[0]) {
            return false;
        }
    }
    return true;
}

} // namespace

std::expected<Cpf, nl::base::Erro> Cpf::de(QStringView entrada)
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
            return std::unexpected(nl::base::Erro{"cpf.formato", std::nullopt,
                                                  "CPF aceita só dígitos, sem letra nem barra"});
        }
    }
    if (digitos.size() != kTamanho) {
        return std::unexpected(nl::base::Erro{"cpf.tamanho", std::nullopt, "CPF deve ter 11 dígitos"});
    }
    if (todosIguais(digitos)) {
        return std::unexpected(nl::base::Erro{"cpf.repetido", std::nullopt,
                                              "CPF com todos os dígitos iguais não é válido"});
    }

    const QStringView vista{digitos};
    const int dv1 = digitoVerificador(vista.first(9));
    const int dv2 = digitoVerificador(vista.first(10));
    if (static_cast<int>(digitos.at(9).unicode()) - 48 != dv1 ||
        static_cast<int>(digitos.at(10).unicode()) - 48 != dv2) {
        return std::unexpected(nl::base::Erro{"cpf.dv", std::nullopt, "dígito verificador do CPF não confere"});
    }
    return Cpf{digitos};
}

QString Cpf::texto() const
{
    return texto_;
}

QString Cpf::comMascara() const
{
    return QStringLiteral("%1.%2.%3-%4").arg(texto_.mid(0, 3), texto_.mid(3, 3), texto_.mid(6, 3), texto_.mid(9, 2));
}

Cpf::Cpf(QString normalizado) : texto_{std::move(normalizado)}
{
}

} // namespace nl::back
