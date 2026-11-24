// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Modo fixado (NL-3.1a, bloco I-G): verifica o servidor pela impressão SHA-256 dada (ADR 0008). Para o MySQL (classe A)
// e para o MariaDB com exigência de transporte seguro. A impressão é conferida pela validação antes de chegar aqui.

#include "nl/foundation/sql/estado.h"

#include <mysql.h>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> aplicarModoFixado(MYSQL* conexao, const QByteArray& impressao)
{
    my_bool verifica = 1;
    if (mysql_optionsv(conexao, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verifica) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível exigir a verificação do certificado"));
    }
    // Cópia NUL-terminada da impressão: o conector faz cópia própria (strdup), conferida por tst_sql_opcoes_tls.
    const QByteArray impressaoNul = impressao;
    if (mysql_optionsv(conexao, MARIADB_OPT_TLS_PEER_FP, impressaoNul.constData()) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível fixar a impressão do certificado"));
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
