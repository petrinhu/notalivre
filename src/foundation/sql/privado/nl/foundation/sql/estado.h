// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Estado interno do Executor e a API interna entre os átomos do motor sql (NL-3.1a, bloco I-G). O MYSQL só aparece
// aqui e nos .cpp do motor. Um átomo por arquivo: cada função abaixo é definida num único .cpp.

#ifndef NL_FOUNDATION_SQL_PRIVADO_ESTADO_H
#define NL_FOUNDATION_SQL_PRIVADO_ESTADO_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/dados_conexao.h"
#include "nl/foundation/sql/executor.h"

#include <mysql.h>

#include <expected>
#include <functional>

namespace nl::foundation::sql {

struct Executor::Estado {
    explicit Estado(MYSQL* conexao) : mysql(conexao) {}
    ~Estado()
    {
        if (mysql != nullptr) {
            mysql_close(mysql);
        }
    }
    Estado(const Estado&) = delete;
    Estado& operator=(const Estado&) = delete;

    MYSQL* mysql = nullptr;
    bool emTransacao = false;
};

namespace detalhe {

// Único acesso interno ao estado do Executor (v1j, decisão 1): o friend do executor.h aponta para este struct.
struct AcessoAoEstado {
    static MYSQL* mysql(const Executor& executor) { return executor.estado_->mysql; }
    static Executor::Estado& estado(Executor& executor) { return *executor.estado_; }
};

[[nodiscard]] int tentativasDeConexao() noexcept;             // conexao.cpp: a costura de teste lê o contador
void zerarTentativasDeConexao() noexcept;                     // conexao.cpp
[[nodiscard]] std::expected<MYSQL*, nl::base::Erro> abrirSessao(const DadosConexao& dados);  // conexao.cpp
[[nodiscard]] std::expected<void, nl::base::Erro> aplicarOpcoesSessao(MYSQL* conexao, const DadosConexao& dados); // opcoes_sessao.cpp
[[nodiscard]] std::expected<void, nl::base::Erro> aplicarModoAberto(MYSQL* conexao);     // modo_aberto.cpp
[[nodiscard]] std::expected<void, nl::base::Erro> aplicarModoFixado(MYSQL* conexao, const QByteArray& impressao); // modo_fixado.cpp
[[nodiscard]] std::expected<void, nl::base::Erro> reexecutarSemVinculo(MYSQL* conexao);  // preparado.cpp
[[nodiscard]] std::expected<void, nl::base::Erro> executarTransacao(MYSQL* conexao, bool& emTransacao, Executor& executor, const std::function<std::expected<void, nl::base::Erro>(Executor&)>& corpo); // transacao.cpp

[[nodiscard]] nl::base::Erro erroDeConexao(unsigned erro, const char* mensagem);   // erros.cpp
[[nodiscard]] nl::base::Erro erroDePreparo(unsigned erro, const char* mensagem);   // erros.cpp
[[nodiscard]] nl::base::Erro erroDeExecucao(unsigned erro, const char* mensagem);  // erros.cpp
[[nodiscard]] nl::base::Erro erroDeLeitura(unsigned erro, const char* mensagem);   // erros.cpp
[[nodiscard]] nl::base::Erro erroDeOpcao(const char* mensagem);                    // erros.cpp

} // namespace detalhe
} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_PRIVADO_ESTADO_H
