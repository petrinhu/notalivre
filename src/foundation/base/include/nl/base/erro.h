// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Erro da camada nl_foundation_base (NL-1.12): código estável, campo do leiaute opcional e mensagem em pt-br.
// Sem Qt: o tipo é usado por std::expected<T, Erro> em toda a Back.

#ifndef NL_BASE_ERRO_H
#define NL_BASE_ERRO_H

#include <optional>
#include <string>

namespace nl::base {

/// Erro de domínio. Igualdade compara só código e campo do leiaute; a mensagem é apresentação.
class Erro
{
public:
    Erro(std::string codigo, std::optional<std::string> campo, std::string mensagem);

    /// Código estável, por exemplo "cnpj.formato" ou "E0188". Não muda com o texto da mensagem.
    const std::string& codigo() const noexcept { return codigo_; }

    /// Campo do leiaute (por exemplo "Dps.Prestador.CNPJ"), quando o erro se refere a um campo.
    const std::optional<std::string>& campo() const noexcept { return campo_; }

    /// Mensagem em pt-br, para o usuário e para o log.
    const std::string& mensagem() const noexcept { return mensagem_; }

private:
    std::string codigo_;
    std::optional<std::string> campo_;
    std::string mensagem_;
};

/// Mesmo código e mesmo campo (ou ambos sem campo) são o mesmo erro, mesmo com mensagens diferentes.
bool operator==(const Erro& esquerdo, const Erro& direito) noexcept;

} // namespace nl::base

#endif // NL_BASE_ERRO_H
