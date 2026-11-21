// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Preparado: RAII do MYSQL_STMT com o flag vinculado_ próprio (NL-3.1a, bloco I-G). Cada execução exige um vincular
// novo; sem ele, executar devolve sql.sem_vinculo sem chamar o conector.

#ifndef NL_FOUNDATION_SQL_PRIVADO_PREPARADO_H
#define NL_FOUNDATION_SQL_PRIVADO_PREPARADO_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/parametro.h"
#include "nl/foundation/sql/vinculos.h"

#include <mysql.h>

#include <QByteArray>
#include <QtGlobal>

#include <expected>
#include <span>

namespace nl::foundation::sql::detalhe {

class Preparado {
public:
    [[nodiscard]] static std::expected<Preparado, nl::base::Erro> preparar(MYSQL* conexao, const QByteArray& texto);

    Preparado(Preparado&& outro) noexcept;
    Preparado& operator=(Preparado&& outro) noexcept;
    Preparado(const Preparado&) = delete;
    Preparado& operator=(const Preparado&) = delete;
    ~Preparado();

    [[nodiscard]] std::expected<void, nl::base::Erro> vincular(std::span<const Parametro> parametros);
    [[nodiscard]] std::expected<qint64, nl::base::Erro> executar();   // linhas afetadas; exige vincular antes
    [[nodiscard]] MYSQL_STMT* stmt() const noexcept { return stmt_; }

private:
    explicit Preparado(MYSQL_STMT* stmt) : stmt_(stmt) {}
    MYSQL_STMT* stmt_ = nullptr;
    bool vinculado_ = false;
    VinculosMontados vinculos_;
};

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_PREPARADO_H
