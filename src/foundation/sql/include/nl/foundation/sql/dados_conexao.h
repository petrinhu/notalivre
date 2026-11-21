// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Dados de conexão da Foundation sql (NL-3.1a): o modo de TLS é obrigatório, sem padrão.

#ifndef NL_FOUNDATION_SQL_DADOS_CONEXAO_H
#define NL_FOUNDATION_SQL_DADOS_CONEXAO_H

#include <QByteArray>

#include <utility>
#include <variant>

namespace nl::foundation::sql {
struct ModoAberto {};
struct ModoFixado {
    explicit ModoFixado(QByteArray impressaoSha256Hex) : impressaoSha256Hex(std::move(impressaoSha256Hex)) {}
    QByteArray impressaoSha256Hex;   // 64 dígitos hex; conectar recusa outro formato
};
using ModoTls = std::variant<ModoFixado, ModoAberto>;   // ModoFixado sem construtor padrão: ModoTls sem valor não compila
struct DadosConexao {
    QByteArray host;                          // só "127.0.0.1" ou "::1"; "localhost", vazio e qualquer outro: recusa
    int porta = 0;                            // 1 a 65535
    QByteArray banco;
    QByteArray usuario;
    const char* senhaNulTerminada = nullptr;  // SenhaSegura::comoC(); o motor não copia; nulo: recusa
    QByteArray pluginsPermitidos;             // nomes separados por vírgula, conferidos por conjunto; vazio: recusa
    ModoTls modo;                             // obrigatório, sem padrão
};
} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_DADOS_CONEXAO_H
