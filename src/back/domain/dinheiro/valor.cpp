// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do valor monetário da camada nl_back (NL-1.2): centavos em int64, parse pt-br estrito.

#include <nl/back/dinheiro/valor.h>

#include <nl/base/erro.h>

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include <QChar>
#include <QList>
#include <QString>
#include <QStringView>

namespace nl::back {
namespace {

// Reais do teto do leiaute: 15 dígitos inteiros. Com duas casas, centavos do teto = reais * 100 + 99.
constexpr std::int64_t tetoReais = 999'999'999'999'999;

using Erro = nl::base::Erro;
using ResultadoReais = std::expected<std::int64_t, Erro>;

Erro erroDe(std::string codigo, std::string mensagem)
{
    return Erro{std::move(codigo), std::nullopt, std::move(mensagem)};
}

std::unexpected<Erro> falha(std::string codigo, std::string mensagem)
{
    return std::unexpected{erroDe(std::move(codigo), std::move(mensagem))};
}

// Dígito ASCII apenas. QChar::isDigit() aceitaria dígitos de outros alfabetos.
bool ehDigito(char16_t c) noexcept
{
    return c >= u'0' && c <= u'9';
}

// Parte inteira pt-br: "0", "1234" ou "1.234.567" (ponto só entre grupos de exatamente três dígitos).
ResultadoReais lerReais(QStringView inteira)
{
    if (inteira.isEmpty()) {
        return falha("valor.formato", "formato de valor inválido: informe os reais antes da vírgula, por exemplo 1.234,56");
    }
    for (QChar c : inteira) {
        if (c.unicode() != u'.' && !ehDigito(c.unicode())) {
            return falha("valor.formato", "formato de valor inválido: use dígitos, vírgula para os centavos e ponto só como separador de milhar");
        }
    }

    const QList<QStringView> grupos = inteira.split(u'.');
    if (grupos.size() > 1) {
        const QStringView primeiro = grupos.first();
        const bool primeiroValido = primeiro.size() >= 1 && primeiro.size() <= 3 && primeiro.front().unicode() != u'0';
        if (!primeiroValido) {
            return falha("valor.separador", "separador de milhar inválido: o primeiro grupo tem de 1 a 3 dígitos e não começa com zero");
        }
        for (qsizetype i = 1; i < grupos.size(); ++i) {
            if (grupos[i].size() != 3) {
                return falha("valor.separador", "separador de milhar inválido: use ponto só entre grupos de três dígitos (1.234,56)");
            }
        }
    } else if (inteira.size() > 1 && inteira.front().unicode() == u'0') {
        return falha("valor.formato", "formato de valor inválido: não use zero à esquerda (use 0 ou comece pelo primeiro dígito significativo)");
    }

    // Acumula dígito a dígito e confere o teto ANTES de multiplicar: nunca passa de int64.
    std::int64_t reais = 0;
    for (QChar c : inteira) {
        if (c.unicode() == u'.') {
            continue;
        }
        const auto digito = static_cast<std::int64_t>(c.unicode() - u'0');
        if (reais > (tetoReais - digito) / 10) {
            return falha("valor.limite", "valor acima do limite do leiaute (999.999.999.999.999,99)");
        }
        reais = reais * 10 + digito;
    }
    return reais;
}

// Parte decimal pt-br: um ou dois dígitos ASCII, lidos como centavos (1 -> 10, 05 -> 5).
std::expected<std::int64_t, Erro> lerCentavos(QStringView decimal)
{
    if (decimal.isEmpty()) {
        return falha("valor.formato", "formato de valor inválido: depois da vírgula informe um ou dois dígitos");
    }
    for (QChar c : decimal) {
        if (!ehDigito(c.unicode())) {
            return falha("valor.formato", "formato de valor inválido: use dígitos, vírgula para os centavos e ponto só como separador de milhar");
        }
    }
    if (decimal.size() > 2) {
        return falha("valor.casas", "valor com mais de duas casas decimais: o leiaute aceita no máximo duas");
    }
    std::int64_t centavos = 0;
    for (QChar c : decimal) {
        centavos = centavos * 10 + static_cast<std::int64_t>(c.unicode() - u'0');
    }
    return decimal.size() == 1 ? centavos * 10 : centavos;
}

} // namespace

std::expected<Valor, Erro> Valor::de(std::int64_t centavos)
{
    if (centavos < 0) {
        return falha("valor.negativo", "valor negativo: o leiaute não aceita sinal de menos");
    }
    if (centavos > teto) {
        return falha("valor.limite", "valor acima do limite do leiaute (999.999.999.999.999,99)");
    }
    return Valor{centavos};
}

std::expected<Valor, Erro> Valor::deTextoPtBr(QStringView texto)
{
    if (texto.isEmpty()) {
        return falha("valor.vazio", "valor vazio: informe o valor, por exemplo 1.234,56");
    }
    if (texto.startsWith(u'-')) {
        return falha("valor.negativo", "valor negativo: o leiaute não aceita sinal de menos");
    }

    const qsizetype virgula = texto.indexOf(u',');
    const QStringView inteira = virgula < 0 ? texto : texto.first(virgula);
    if (virgula >= 0 && texto.indexOf(u',', virgula + 1) >= 0) {
        return falha("valor.formato", "formato de valor inválido: use uma única vírgula, só para os centavos");
    }

    const ResultadoReais reais = lerReais(inteira);
    if (!reais) {
        return std::unexpected{reais.error()};
    }

    std::int64_t centavos = *reais * 100;
    if (virgula >= 0) {
        const std::expected<std::int64_t, Erro> frac = lerCentavos(texto.sliced(virgula + 1));
        if (!frac) {
            return std::unexpected{frac.error()};
        }
        centavos += *frac;
    }
    // Pelo teto dos reais, centavos <= teto. Construtor privado: nenhum valor fora do intervalo entra aqui.
    return Valor{centavos};
}

std::expected<Valor, Erro> Valor::somaDeLote(std::span<const Valor> lote)
{
    std::int64_t total = 0;
    for (const Valor& valor : lote) {
        // total e valor.centavos_ estão em [0, teto]: teto - total não estoura, e a comparação decide antes de somar.
        if (valor.centavos_ > teto - total) {
            return falha("valor.limite", "soma do lote acima do limite do leiaute (999.999.999.999.999,99)");
        }
        total += valor.centavos_;
    }
    return Valor{total};
}

QString Valor::paraLeiaute() const
{
    const std::int64_t reais = centavos_ / 100;
    const std::int64_t cent = centavos_ % 100;
    return QStringLiteral("%1.%2").arg(reais).arg(cent, 2, 10, QChar{u'0'});
}

} // namespace nl::back
