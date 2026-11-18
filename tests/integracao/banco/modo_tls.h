// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Modo de TLS de uma conexão de teste (NL-3.1h, critério 4 de TLS, C4'): padrão da biblioteca, aberto (sem verificação e
// sem obrigação) e fixado (verifica o servidor pela impressão SHA-256). Nunca mysql_ssl_set nem ssl_*: com a verificação
// desligada, mandam a senha em claro (armadilha B do CISO, 1.2).
#ifndef NL_TESTE_MODO_TLS_H
#define NL_TESTE_MODO_TLS_H

#include <mariadb/mysql.h>

#include <QByteArray>

namespace {

enum class ModoTls { Padrao, Aberto, Fixado };

// Padrão: nenhuma opção de TLS. É registro da biblioteca, não modo do produto.
// Aberto: sem verificação do certificado e sem obrigação de TLS (MYSQL_OPT_SSL_ENFORCE=0).
// Fixado: confere a impressão SHA-256 dada e exige a verificação.
void aplicarModoTls(MYSQL *conexao, ModoTls modo, const QByteArray &impressao)
{
    switch (modo) {
    case ModoTls::Padrao:
        return;
    case ModoTls::Aberto: {
        my_bool desligado = 0;
        mysql_optionsv(conexao, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &desligado);
        mysql_optionsv(conexao, MYSQL_OPT_SSL_ENFORCE, &desligado);
        return;
    }
    case ModoTls::Fixado: {
        my_bool ligado = 1;
        mysql_optionsv(conexao, MARIADB_OPT_TLS_PEER_FP, impressao.constData());
        mysql_optionsv(conexao, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &ligado);
        return;
    }
    }
}

} // namespace

#endif // NL_TESTE_MODO_TLS_H
