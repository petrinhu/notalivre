// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Log estruturado (JSON por linha) da Foundation, com redação no ponto de escrita (NL-2.7, ADR-S4).
// Sem singleton: o destino e a allowlist entram pelo construtor. Erro por std::expected (convenção do projeto).

#ifndef NL_FOUNDATION_LOG_LOG_H
#define NL_FOUNDATION_LOG_LOG_H

#include <nl/base/erro.h>
#include <nl/foundation/log/redacao.h>

#include <cstddef>
#include <expected>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace nl::foundation::log {

enum class Nivel {
    Info,
    Aviso,
    Erro,
};

/// Campo declarado na allowlist. Nome que não está na lista é descartado: falha fechada (ADR-S4, seção 3).
struct DeclaracaoCampo {
    std::string nome;
    Regra regra;
};

/// Campo de um registro: nome e valor ainda não redigidos. Quem decide se ele sai é o Log.
struct Campo {
    std::string nome;
    std::string valor;
};

class Log {
public:
    Log(std::ostream& destino, std::string servico, std::vector<DeclaracaoCampo> allowlist);

    /// Grava uma linha JSON com nível, carimbo UTC, serviço, evento e os campos permitidos. Campo redigido ou
    /// descartado não deixa o valor na saída. Devolve erro só se a escrita no destino falhar.
    [[nodiscard]] std::expected<void, nl::base::Erro> registrar(Nivel nivel, std::string_view evento,
                                                                const std::vector<Campo>& campos);

    /// Total de campos descartados desde a criação (não declarado, proibido ou fora do formato).
    [[nodiscard]] std::size_t descartados() const noexcept;

private:
    /// Valor pronto para gravar, ou nullopt se o campo deve ser descartado.
    [[nodiscard]] std::optional<std::string> valorGravavel(const Campo& campo) const;

    std::ostream& destino_;
    std::string servico_;
    std::vector<DeclaracaoCampo> allowlist_;
    std::size_t descartados_{0};
};

} // namespace nl::foundation::log

#endif // NL_FOUNDATION_LOG_LOG_H
