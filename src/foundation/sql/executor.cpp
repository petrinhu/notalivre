// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco U-R (NL-3.1a): o executor entra no I-G. Toda operação devolve sql.nao_implementado.

#include "nl/foundation/sql/executor.h"

#include <optional>
#include <utility>

namespace nl::foundation::sql {

struct Executor::Estado {};

namespace {

nl::base::Erro naoImplementado()
{
    return nl::base::Erro("sql.nao_implementado", std::nullopt, "não implementado");
}

} // namespace

std::expected<Executor, nl::base::Erro> Executor::conectar(const DadosConexao&)
{
    return std::unexpected(naoImplementado());
}

Executor::Executor(Executor&&) noexcept = default;
Executor& Executor::operator=(Executor&&) noexcept = default;
Executor::~Executor() = default;

std::expected<Linhas, nl::base::Erro> Executor::consultar(const Consulta&, std::span<const Parametro>)
{
    return std::unexpected(naoImplementado());
}

std::expected<qint64, nl::base::Erro> Executor::executar(const Consulta&, std::span<const Parametro>)
{
    return std::unexpected(naoImplementado());
}

std::expected<void, nl::base::Erro> Executor::executarDdl(const Ddl&)
{
    return std::unexpected(naoImplementado());
}

std::expected<void, nl::base::Erro> Executor::transacao(const std::function<std::expected<void, nl::base::Erro>(Executor&)>&)
{
    return std::unexpected(naoImplementado());
}

QByteArray Executor::cifraTlsDaSessao() const
{
    return {};
}

} // namespace nl::foundation::sql
