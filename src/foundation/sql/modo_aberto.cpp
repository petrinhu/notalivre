// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Modo aberto (NL-3.1a, bloco I-G): sem verificação do certificado e sem obrigação de TLS. Só para o MariaDB sem TLS
// (classe B) e para o MariaDB com TLS em memória (classe C), que o perfil aberto cobre. Este é o único lugar do motor que
// desliga a obrigação de TLS (portão sql_seguro, regra S4).

#include "nl/foundation/sql/estado.h"

#include <mysql.h>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> aplicarModoAberto(MYSQL* conexao)
{
    my_bool desligado = 0;
    if (mysql_optionsv(conexao, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &desligado) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível desligar a verificação do certificado"));
    }
    if (mysql_optionsv(conexao, MYSQL_OPT_SSL_ENFORCE, &desligado) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível desligar a obrigação de TLS"));
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
