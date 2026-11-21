// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Preparado (NL-3.1a, bloco I-G): RAII do MYSQL_STMT. Preparar põe vinculado_ em falso; vincular põe em verdadeiro; executar
// exige o verdadeiro e o volta a falso. Assim, reexecutar sem vincular de novo devolve sql.sem_vinculo sem ir ao conector.

#include "nl/foundation/sql/estado.h"
#include "nl/foundation/sql/preparado.h"

#include <mysql.h>

#include <optional>
#include <utility>

namespace nl::foundation::sql::detalhe {

std::expected<Preparado, nl::base::Erro> Preparado::preparar(MYSQL* conexao, const QByteArray& texto)
{
    MYSQL_STMT* stmt = mysql_stmt_init(conexao);
    if (stmt == nullptr) {
        return std::unexpected(erroDePreparo(mysql_errno(conexao), mysql_error(conexao)));
    }
    if (mysql_stmt_prepare(stmt, texto.constData(), static_cast<unsigned long>(texto.size())) != 0) {
        nl::base::Erro erro = erroDePreparo(mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return std::unexpected(erro);
    }
    return Preparado(stmt);
}

Preparado::Preparado(Preparado&& outro) noexcept
    : stmt_(std::exchange(outro.stmt_, nullptr)),
      vinculado_(std::exchange(outro.vinculado_, false)),
      vinculos_(std::move(outro.vinculos_))
{
}

Preparado& Preparado::operator=(Preparado&& outro) noexcept
{
    if (this != &outro) {
        if (stmt_ != nullptr) {
            mysql_stmt_free_result(stmt_);
            mysql_stmt_close(stmt_);
        }
        stmt_ = std::exchange(outro.stmt_, nullptr);
        vinculado_ = std::exchange(outro.vinculado_, false);
        vinculos_ = std::move(outro.vinculos_);
    }
    return *this;
}

Preparado::~Preparado()
{
    if (stmt_ != nullptr) {
        mysql_stmt_free_result(stmt_);
        mysql_stmt_close(stmt_);
    }
}

std::expected<void, nl::base::Erro> Preparado::vincular(std::span<const Parametro> parametros)
{
    vinculado_ = false;
    if (static_cast<std::size_t>(mysql_stmt_param_count(stmt_)) != parametros.size()) {
        return std::unexpected(nl::base::Erro("sql.vinculo_quantidade", std::nullopt,
                                              "a quantidade de parâmetros não bate com os marcadores"));
    }
    auto montados = montarVinculos(parametros);
    if (!montados) {
        return std::unexpected(montados.error());
    }
    vinculos_ = std::move(*montados);
    if (!vinculos_.vinculos.empty() && mysql_stmt_bind_param(stmt_, vinculos_.vinculos.data()) != 0) {
        return std::unexpected(nl::base::Erro("sql.vinculo_quantidade", std::nullopt,
                                              "o conector recusou o vínculo dos parâmetros"));
    }
    vinculado_ = true;
    return {};
}

std::expected<qint64, nl::base::Erro> Preparado::executar()
{
    if (!vinculado_) {
        return std::unexpected(nl::base::Erro("sql.sem_vinculo", std::nullopt,
                                              "a execução pediu parâmetros sem vincular de novo"));
    }
    vinculado_ = false;
    if (mysql_stmt_execute(stmt_) != 0) {
        return std::unexpected(erroDeExecucao(mysql_stmt_errno(stmt_), mysql_stmt_error(stmt_)));
    }
    return static_cast<qint64>(mysql_stmt_affected_rows(stmt_));
}

std::expected<void, nl::base::Erro> reexecutarSemVinculo(MYSQL* conexao)
{
    auto preparado = Preparado::preparar(conexao, QByteArrayLiteral("SELECT ?"));
    if (!preparado) {
        return std::unexpected(preparado.error());
    }
    auto executado = preparado->executar();
    if (!executado) {
        return std::unexpected(executado.error());
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
