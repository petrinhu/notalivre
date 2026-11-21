// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Transação (NL-3.1a, bloco I-G): autocommit desligado, o corpo roda, e o resultado do corpo decide COMMIT ou ROLLBACK.
// Uma transação por executor; a transação aninhada é recusada com sql.transacao_aninhada.

#include "nl/foundation/sql/estado.h"

#include <mysql.h>

#include <optional>
#include <string>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> executarTransacao(MYSQL* conexao, bool& emTransacao, Executor& executor, const std::function<std::expected<void, nl::base::Erro>(Executor&)>& corpo)
{
    if (emTransacao) {
        return std::unexpected(nl::base::Erro("sql.transacao_aninhada", std::nullopt, "transação dentro de transação"));
    }
    if (mysql_autocommit(conexao, 0) != 0) {
        return std::unexpected(nl::base::Erro("sql.transacao", std::nullopt, mysql_error(conexao)));
    }
    emTransacao = true;
    std::expected<void, nl::base::Erro> resultado = corpo(executor);
    if (resultado) {
        if (mysql_commit(conexao) != 0) {
            resultado = std::unexpected(nl::base::Erro("sql.transacao", std::nullopt, mysql_error(conexao)));
        }
    } else {
        mysql_rollback(conexao);
    }
    emTransacao = false;
    mysql_autocommit(conexao, 1);
    return resultado;
}

} // namespace nl::foundation::sql::detalhe
