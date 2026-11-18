// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Estado de TLS de uma sessão do banco, lido dos dois lados (NL-3.1h, critério 4 da fatia): o cliente pela API do
// Connector/C e o servidor pela variável de status Ssl_cipher da própria sessão.
#ifndef NL_TESTE_TLS_DA_SESSAO_H
#define NL_TESTE_TLS_DA_SESSAO_H

#include <mariadb/mysql.h>

#include <QString>

namespace {

// Lado do cliente: a sessão tem cifra de TLS. Sem pedido de TLS, a API devolve nulo.
bool tlsDoCliente(MYSQL *conexao)
{
    return mysql_get_ssl_cipher(conexao) != nullptr;
}

// Lado do servidor: o valor de Ssl_cipher da sessão. Vazio quando a sessão não usa TLS.
// Devolve "ERRO" se a consulta de status falhar, para que o teste nunca leia isso como "sem TLS".
QString tlsDoServidor(MYSQL *conexao)
{
    if (mysql_query(conexao, "SHOW SESSION STATUS LIKE 'Ssl_cipher'") != 0) {
        return QStringLiteral("ERRO");
    }
    MYSQL_RES *resultado = mysql_store_result(conexao);
    if (resultado == nullptr) {
        return QStringLiteral("ERRO");
    }
    QString cifra;
    if (MYSQL_ROW linha = mysql_fetch_row(resultado)) {
        cifra = linha[1] != nullptr ? QString::fromUtf8(linha[1]) : QString();
    }
    mysql_free_result(resultado);
    return cifra;
}

} // namespace

#endif // NL_TESTE_TLS_DA_SESSAO_H
