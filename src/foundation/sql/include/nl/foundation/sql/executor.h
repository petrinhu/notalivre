// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Executor da Foundation sql (NL-3.1a): a única porta de SQL; MYSQL fica no .cpp.

#ifndef NL_FOUNDATION_SQL_EXECUTOR_H
#define NL_FOUNDATION_SQL_EXECUTOR_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/consulta.h"
#include "nl/foundation/sql/dados_conexao.h"
#include "nl/foundation/sql/parametro.h"

#include <QByteArray>
#include <expected>
#include <functional>
#include <memory>
#include <span>

namespace nl::foundation::sql {
class Executor {
public:
    [[nodiscard]] static std::expected<Executor, nl::base::Erro> conectar(const DadosConexao& dados);
    Executor(Executor&&) noexcept;
    Executor& operator=(Executor&&) noexcept;
    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;
    ~Executor();
    [[nodiscard]] std::expected<Linhas, nl::base::Erro> consultar(const Consulta& consulta, std::span<const Parametro> parametros);
    [[nodiscard]] std::expected<qint64, nl::base::Erro> executar(const Consulta& consulta, std::span<const Parametro> parametros);
    [[nodiscard]] std::expected<void, nl::base::Erro> executarDdl(const Ddl& ddl);   // recusa dentro de transação
    [[nodiscard]] std::expected<void, nl::base::Erro> transacao(
        const std::function<std::expected<void, nl::base::Erro>(Executor&)>& corpo);   // erro do corpo: rollback
    [[nodiscard]] QByteArray cifraTlsDaSessao() const;   // vazio: sessão sem TLS (mysql_get_ssl_cipher)
private:
    struct Estado;
    explicit Executor(std::unique_ptr<Estado> estado);
    std::unique_ptr<Estado> estado_;   // MYSQL* mora só no .cpp
};
} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_EXECUTOR_H
