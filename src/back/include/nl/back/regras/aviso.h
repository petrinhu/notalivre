// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Aviso do site (NL-1.5, ponto L10): regra só do Emissor Web, não bloqueante. Tipo próprio, separado de nl::base::Erro.
#ifndef NL_BACK_REGRAS_AVISO_H
#define NL_BACK_REGRAS_AVISO_H

#include <string>
#include <utility>

namespace nl::back {

/// Código D (D1, D3, D4), campo do leiaute e mensagem em pt-br. Um aviso nunca impede emitir.
class Aviso
{
public:
    Aviso(std::string codigo, std::string campo, std::string mensagem)
        : codigo_{std::move(codigo)}, campo_{std::move(campo)}, mensagem_{std::move(mensagem)}
    {
    }

    const std::string& codigo() const noexcept { return codigo_; }
    const std::string& campo() const noexcept { return campo_; }
    const std::string& mensagem() const noexcept { return mensagem_; }

    bool operator==(const Aviso& outro) const = default;

private:
    std::string codigo_;
    std::string campo_;
    std::string mensagem_;
};

} // namespace nl::back

#endif // NL_BACK_REGRAS_AVISO_H
