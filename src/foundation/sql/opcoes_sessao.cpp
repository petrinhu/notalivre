// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Opções de sessão (NL-3.1a, bloco I-G, tabela 1.5): uma chamada por opção, retorno conferido. Este é o único arquivo
// do motor que cita as seis opções de sessão, uma vez cada (portão sql_seguro, regra S2). Ordem fixa, antes do modo.

#include "nl/foundation/sql/estado.h"

#include <mysql.h>

namespace nl::foundation::sql::detalhe {

std::expected<void, nl::base::Erro> aplicarOpcoesSessao(MYSQL* conexao, const DadosConexao& dados)
{
    // Sem LOAD DATA LOCAL: o cliente não pode enviar arquivo local.
    unsigned int semArquivoLocal = 0;
    if (mysql_optionsv(conexao, MYSQL_OPT_LOCAL_INFILE, &semArquivoLocal) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível desligar o envio de arquivo local"));
    }
    // Sem reconexão automática: uma queda vira erro na chamada seguinte.
    my_bool semReconexao = 0;
    if (mysql_optionsv(conexao, MYSQL_OPT_RECONNECT, &semReconexao) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível desligar a reconexão automática"));
    }
    // Sessão com o modo estrito e o fuso fixo; o literal é o mesmo da tabela 1.5.
    const char comandoInicial[] = "SET SESSION sql_mode='STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION', time_zone='+00:00', "
                                  "collation_connection='utf8mb4_bin'";
    if (mysql_optionsv(conexao, MYSQL_INIT_COMMAND, comandoInicial) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível fixar o comando inicial da sessão"));
    }
    // Só os métodos da lista do perfil: a lista já foi conferida pela validação.
    if (mysql_optionsv(conexao, MARIADB_OPT_RESTRICTED_AUTH, dados.pluginsPermitidos.constData()) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível restringir os métodos de autenticação"));
    }
    enum mysql_protocol_type protocolo = MYSQL_PROTOCOL_TCP;
    if (mysql_optionsv(conexao, MYSQL_OPT_PROTOCOL, &protocolo) != 0) {
        return std::unexpected(erroDeOpcao("não foi possível fixar o protocolo TCP"));
    }
    if (mysql_optionsv(conexao, MYSQL_SET_CHARSET_NAME, "utf8mb4") != 0) {
        return std::unexpected(erroDeOpcao("não foi possível fixar o conjunto de caracteres utf8mb4"));
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
