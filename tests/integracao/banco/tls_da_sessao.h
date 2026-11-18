// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Estado de TLS de uma sessão do banco, lido dos dois lados (NL-3.1h, critério 4 da fatia): o cliente pela API do
// Connector/C e o servidor pela variável de status da própria sessão. Cada leitura devolve texto, nunca bool: "nulo",
// "vazio", "ERRO" e "nd" são valores que o teste compara, e nenhum deles é lido como TLS ativo.
#ifndef NL_TESTE_TLS_DA_SESSAO_H
#define NL_TESTE_TLS_DA_SESSAO_H

#include <mariadb/mysql.h>

#include <QString>

namespace {

// Lado do cliente: a cifra da sessão, ou "nulo" quando a API não tem cifra (sem TLS).
QString tlsDoCliente(MYSQL *conexao)
{
    const char *cifra = mysql_get_ssl_cipher(conexao);
    return cifra != nullptr ? QString::fromUtf8(cifra) : QStringLiteral("nulo");
}

// Lê uma variável de status de sessão (Ssl_cipher ou Ssl_version). Vazio quando a sessão não a tem; "ERRO" se a
// consulta falhar, para que o teste nunca leia isso como "sem TLS".
QString statusDeSessao(MYSQL *conexao, const char *consulta)
{
    if (mysql_query(conexao, consulta) != 0) {
        return QStringLiteral("ERRO");
    }
    MYSQL_RES *resultado = mysql_store_result(conexao);
    if (resultado == nullptr) {
        return QStringLiteral("ERRO");
    }
    QString valor = QStringLiteral("vazio");
    if (MYSQL_ROW linha = mysql_fetch_row(resultado)) {
        if (linha[1] != nullptr && linha[1][0] != '\0') {
            valor = QString::fromUtf8(linha[1]);
        }
    }
    mysql_free_result(resultado);
    return valor;
}

// Lado do servidor: o valor de Ssl_cipher da sessão.
QString tlsDoServidor(MYSQL *conexao)
{
    return statusDeSessao(conexao, "SHOW SESSION STATUS LIKE 'Ssl_cipher'");
}

// Versão do TLS negociada na sessão (Ssl_version), ou "vazio" sem TLS.
QString versaoTlsDoServidor(MYSQL *conexao)
{
    return statusDeSessao(conexao, "SHOW SESSION STATUS LIKE 'Ssl_version'");
}

// Estado da verificação do certificado pela biblioteca (MARIADB_TLS_VERIFY_STATUS). Só registro, sem valor esperado.
QString statusDaVerificacao(MYSQL *conexao)
{
    unsigned int estado = 0;
    if (mysql_get_optionv(conexao, MARIADB_TLS_VERIFY_STATUS, &estado) != 0) {
        return QStringLiteral("nd");
    }
    return QString::number(estado);
}

} // namespace

#endif // NL_TESTE_TLS_DA_SESSAO_H
