// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fábrica interna dos erros da cripto (NL-3.2b). Mensagem em pt-br e sem bytes da entrada (ADR-0005).

#ifndef NL_FOUNDATION_CRIPTO_PRIVADO_ERROS_H
#define NL_FOUNDATION_CRIPTO_PRIVADO_ERROS_H

#include <nl/base/erro.h>

#include <optional>
#include <string>

namespace nl::foundation::cripto::detalhe {

/// Erro sem campo do leiaute. O código é estável; a mensagem nunca carrega o conteúdo recebido.
inline nl::base::Erro erroCripto(const char* codigo, const char* mensagem)
{
    return nl::base::Erro{std::string{codigo}, std::nullopt, std::string{mensagem}};
}

} // namespace nl::foundation::cripto::detalhe

#endif // NL_FOUNDATION_CRIPTO_PRIVADO_ERROS_H
