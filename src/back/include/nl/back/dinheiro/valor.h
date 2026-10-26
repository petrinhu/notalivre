// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Valor monetário da camada nl_back (NL-1.2): centavos inteiros em int64, sem ponto flutuante.
// Formato de entrada pt-br; formato de saída do leiaute (ponto e duas casas). Erro por std::expected (CLAUDE.md).

#ifndef NL_BACK_DINHEIRO_VALOR_H
#define NL_BACK_DINHEIRO_VALOR_H

#include <nl/base/erro.h>

#include <cstdint>
#include <expected>
#include <span>

#include <QString>
#include <QStringView>

namespace nl::back {

/// Valor em reais guardado como centavos inteiros.
/// Invariante (no construtor privado, só alcançado pelas fábricas): 0 <= centavos <= Valor::teto.
/// O teto é o maior valor do leiaute TSDec15V2 (15 dígitos inteiros, duas casas, sem sinal).
class Valor
{
public:
    /// 999999999999999,99 em centavos.
    static constexpr std::int64_t teto = 99'999'999'999'999'999;

    /// Constrói a partir de centavos. Negativo devolve valor.negativo; acima do teto devolve valor.limite.
    [[nodiscard]] static std::expected<Valor, nl::base::Erro> de(std::int64_t centavos);

    /// Lê texto pt-br: "1.234,56", "10", "10,5". Ponto só como separador de milhar (grupos de três);
    /// até duas casas decimais; sem sinal, sem espaço, sem símbolo, dígitos ASCII.
    [[nodiscard]] static std::expected<Valor, nl::base::Erro> deTextoPtBr(QStringView texto);

    /// Soma de lote sem perda. Se algum passo passar do teto, devolve valor.limite (nunca estouro silencioso).
    [[nodiscard]] static std::expected<Valor, nl::base::Erro> somaDeLote(std::span<const Valor> lote);

    /// Centavos inteiros do valor.
    [[nodiscard]] std::int64_t centavos() const noexcept { return centavos_; }

    /// Texto do leiaute: ponto decimal, duas casas, sem separador de milhar ("1234.56").
    [[nodiscard]] QString paraLeiaute() const;

    bool operator==(const Valor& outro) const noexcept = default;

private:
    explicit Valor(std::int64_t centavos) noexcept : centavos_{centavos} {}

    std::int64_t centavos_;
};

} // namespace nl::back

#endif // NL_BACK_DINHEIRO_VALOR_H
