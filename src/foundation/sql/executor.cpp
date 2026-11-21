// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Executor (NL-3.1a, bloco I-G): porta única de SQL. Orquestra a validação, a conexão, o preparado, a leitura e a transação;
// não tem lógica própria de opção, vínculo ou leitura.

#include "nl/foundation/sql/estado.h"
#include "nl/foundation/sql/leitura.h"
#include "nl/foundation/sql/preparado.h"
#include "nl/foundation/sql/validacao.h"

#include <memory>
#include <optional>
#include <utility>

namespace nl::foundation::sql {

namespace detalhe {


} // namespace detalhe

std::expected<Executor, nl::base::Erro> Executor::conectar(const DadosConexao& dados)
{
    if (auto validos = detalhe::validarDadosConexao(dados); !validos) {
        return std::unexpected(validos.error());
    }
    auto sessao = detalhe::abrirSessao(dados);
    if (!sessao) {
        return std::unexpected(sessao.error());
    }
    return Executor(std::make_unique<Estado>(*sessao));
}

Executor::Executor(Executor&&) noexcept = default;
Executor& Executor::operator=(Executor&&) noexcept = default;
Executor::~Executor() = default;

std::expected<Linhas, nl::base::Erro> Executor::consultar(const Consulta& consulta, std::span<const Parametro> parametros)
{
    auto preparado = detalhe::Preparado::preparar(estado_->mysql, consulta.texto().toByteArray());
    if (!preparado) {
        return std::unexpected(preparado.error());
    }
    if (auto vinculado = preparado->vincular(parametros); !vinculado) {
        return std::unexpected(vinculado.error());
    }
    if (auto executado = preparado->executar(); !executado) {
        return std::unexpected(executado.error());
    }
    return detalhe::lerLinhas(preparado->stmt());
}

std::expected<qint64, nl::base::Erro> Executor::executar(const Consulta& consulta, std::span<const Parametro> parametros)
{
    auto preparado = detalhe::Preparado::preparar(estado_->mysql, consulta.texto().toByteArray());
    if (!preparado) {
        return std::unexpected(preparado.error());
    }
    if (auto vinculado = preparado->vincular(parametros); !vinculado) {
        return std::unexpected(vinculado.error());
    }
    return preparado->executar();
}

std::expected<void, nl::base::Erro> Executor::executarDdl(const Ddl& ddl)
{
    if (estado_->emTransacao) {
        return std::unexpected(nl::base::Erro("sql.ddl_em_transacao", std::nullopt, "DDL não roda dentro de transação"));
    }
    auto preparado = detalhe::Preparado::preparar(estado_->mysql, ddl.texto().toByteArray());
    if (!preparado) {
        return std::unexpected(preparado.error());
    }
    if (auto vinculado = preparado->vincular(std::span<const Parametro>{}); !vinculado) {
        return std::unexpected(vinculado.error());
    }
    if (auto executado = preparado->executar(); !executado) {
        return std::unexpected(executado.error());
    }
    return {};
}

std::expected<void, nl::base::Erro> Executor::transacao(const std::function<std::expected<void, nl::base::Erro>(Executor&)>& corpo)
{
    return detalhe::executarTransacao(estado_->mysql, estado_->emTransacao, *this, corpo);
}

QByteArray Executor::cifraTlsDaSessao() const
{
    const char* cifra = mysql_get_ssl_cipher(estado_->mysql);
    return cifra != nullptr ? QByteArray(cifra) : QByteArray();
}

} // namespace nl::foundation::sql
