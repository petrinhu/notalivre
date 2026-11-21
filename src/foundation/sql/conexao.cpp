// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conexão do motor sql (NL-3.1a, bloco I-G): o único lugar que chama a conexão do conector. Ordem fixa: inicialização,
// opções de sessão, modo de TLS e conexão. Depois de conectar, zera a cópia da senha que o conector guarda.

#include "nl/foundation/sql/costura_conexao.h"
#include "nl/foundation/sql/estado.h"

#include <mysql.h>

#include <atomic>
#include <cstring>
#include <optional>
#include <string.h>
#include <variant>

namespace nl::foundation::sql {

Executor::Estado::~Estado()
{
    if (mysql != nullptr) {
        mysql_close(mysql);
    }
}

namespace {

// Contagem de tentativas de conexão do processo: sobe só depois da validação, e a costura de teste a lê.
std::atomic<int> tentativasDeConexao{0};

} // namespace

namespace detalhe {

int tentativasDeConexao() noexcept { return tentativasDeConexao.load(); }
void zerarTentativasDeConexao() noexcept { tentativasDeConexao.store(0); }

std::expected<MYSQL*, nl::base::Erro> abrirSessao(const DadosConexao& dados)
{
    MYSQL* conexao = mysql_init(nullptr);
    if (conexao == nullptr) {
        return std::unexpected(erroDeConexao(0, "a inicialização do cliente falhou"));
    }
    if (auto opcoes = aplicarOpcoesSessao(conexao, dados); !opcoes) {
        mysql_close(conexao);
        return std::unexpected(opcoes.error());
    }
    std::expected<void, nl::base::Erro> modo = std::holds_alternative<ModoFixado>(dados.modo)
        ? aplicarModoFixado(conexao, std::get<ModoFixado>(dados.modo).impressaoSha256Hex)
        : aplicarModoAberto(conexao);
    if (!modo) {
        mysql_close(conexao);
        return std::unexpected(modo.error());
    }
    tentativasDeConexao.fetch_add(1);
    MYSQL* sessao = mysql_real_connect(conexao, dados.host.constData(), dados.usuario.constData(),
                                       dados.senhaNulTerminada, dados.banco.constData(),
                                       static_cast<unsigned int>(dados.porta), nullptr, 0);
    if (sessao == nullptr) {
        nl::base::Erro erro = erroDeConexao(mysql_errno(conexao), mysql_error(conexao));
        mysql_close(conexao);
        return std::unexpected(erro);
    }
    // A cópia da senha que o conector fez para a conexão fica zerada, sem mudar o ponteiro (pedido do CISO, 20/11).
    if (sessao->passwd != nullptr) {
        explicit_bzero(sessao->passwd, std::strlen(sessao->passwd));
    }
    return sessao;
}

} // namespace detalhe

namespace costura {

st_mysql* sessaoDoConector(const Executor& executor)
{
    return detalhe::AcessoAoEstado::mysql(executor);
}

std::expected<void, nl::base::Erro> reexecutarPreparadoSemVinculo(Executor& executor)
{
    return detalhe::reexecutarSemVinculo(detalhe::AcessoAoEstado::mysql(executor));
}

} // namespace costura
} // namespace nl::foundation::sql
