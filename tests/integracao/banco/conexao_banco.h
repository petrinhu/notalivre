// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conexão com o servidor do par, pelo cliente MariaDB (Connector/C) (NL-3.1h). Abre a sessão por TCP com a senha do
// arquivo, no modo de TLS pedido, e devolve a conexão com o erro e a mensagem da recusa. Quem chama decide o que faz.
#ifndef NL_TESTE_CONEXAO_BANCO_H
#define NL_TESTE_CONEXAO_BANCO_H

#include "ambiente_banco.h"
#include "modo_tls.h"

#include <mariadb/mysql.h>

#include <QByteArray>
#include <QString>

#include <memory>

namespace {

using ConexaoBanco = std::unique_ptr<MYSQL, void (*)(MYSQL *)>;

// Resultado de uma tentativa: a conexão (nula se recusada), o erro de mysql_errno e a mensagem de mysql_error.
// Erro e mensagem são lidos antes de a conexão ser fechada.
struct ResultadoConexao {
    ConexaoBanco conexao;
    unsigned erro = 0;
    QString mensagem;
};

// Abre a sessão do usuário do par no servidor, com o banco base do par, no modo de TLS pedido.
ResultadoConexao conectar(const DadosBanco &dados, const QByteArray &senha, ModoTls modo, const QByteArray &impressao)
{
    ResultadoConexao resultado{ConexaoBanco(mysql_init(nullptr), &mysql_close), 0, QString()};
    if (!resultado.conexao) {
        resultado.mensagem = QStringLiteral("mysql_init falhou");
        return resultado;
    }
    aplicarModoTls(resultado.conexao.get(), modo, impressao);
    const bool aberta = mysql_real_connect(resultado.conexao.get(), dados.host.toUtf8().constData(),
                                           dados.usuario.toUtf8().constData(), senha.constData(),
                                           dados.base.toUtf8().constData(), dados.porta, nullptr, 0) != nullptr;
    if (!aberta) {
        resultado.erro = mysql_errno(resultado.conexao.get());
        resultado.mensagem = QString::fromUtf8(mysql_error(resultado.conexao.get()));
        resultado.conexao.reset();
    }
    return resultado;
}

} // namespace

#endif // NL_TESTE_CONEXAO_BANCO_H
