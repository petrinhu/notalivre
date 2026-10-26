// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ajudantes dos testes da DPS (NL-1.4): valor opcional e código do erro, sem lançar exceção dentro do slot do QtTest.

#ifndef NL_TESTE_BACK_DPS_AJUDANTES_H
#define NL_TESTE_BACK_DPS_AJUDANTES_H

#include <expected>
#include <optional>

#include <QString>

#include <nl/base/erro.h>

namespace teste {

/// Valor do resultado, ou std::nullopt. Não usa value(), que lançaria dentro do slot do QtTest.
template <typename T>
std::optional<T> valorSeOk(const std::expected<T, nl::base::Erro>& resultado)
{
    if (resultado) {
        return *resultado;
    }
    return std::nullopt;
}

/// Código do erro, ou QString vazia quando o resultado é sucesso.
template <typename T>
QString codigoDoErro(const std::expected<T, nl::base::Erro>& resultado)
{
    if (resultado) {
        return QString{};
    }
    return QString::fromStdString(resultado.error().codigo());
}

} // namespace teste

#endif // NL_TESTE_BACK_DPS_AJUDANTES_H
