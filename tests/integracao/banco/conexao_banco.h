// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conexão com o servidor do par, pelo cliente MariaDB (Connector/C) (NL-3.1h). Só abre a sessão por TCP com a senha do
// arquivo; quem chama decide o que faz com ela. A conexão recusada devolve nulo e é fechada aqui.
#ifndef NL_TESTE_CONEXAO_BANCO_H
#define NL_TESTE_CONEXAO_BANCO_H

#include "ambiente_banco.h"

#include <mariadb/mysql.h>

#include <memory>

namespace {

using ConexaoBanco = std::unique_ptr<MYSQL, void (*)(MYSQL *)>;

// Abre a sessão do usuário do par no servidor, com o banco base do par. Devolve nulo se o servidor recusar.
ConexaoBanco conectar(const DadosBanco &dados, const QByteArray &senha)
{
    ConexaoBanco conexao(mysql_init(nullptr), &mysql_close);
    if (!conexao) {
        return conexao;
    }
    const bool aberta = mysql_real_connect(conexao.get(), dados.host.toUtf8().constData(),
                                           dados.usuario.toUtf8().constData(), senha.constData(),
                                           dados.base.toUtf8().constData(), dados.porta, nullptr, 0) != nullptr;
    if (!aberta) {
        conexao.reset();
    }
    return conexao;
}

} // namespace

#endif // NL_TESTE_CONEXAO_BANCO_H
