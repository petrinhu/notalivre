// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Montagem do array MYSQL_BIND a partir dos Parametro (NL-3.1a, bloco U-R). Dono dos buffers e dos length.

#ifndef NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H
#define NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/parametro.h"

#include <mysql.h>

#include <QByteArray>

#include <expected>
#include <span>
#include <vector>

namespace nl::foundation::sql::detalhe {

struct VinculosMontados {
    std::vector<MYSQL_BIND> vinculos;     // um por parâmetro, na ordem do span
    std::vector<unsigned long> tamanhos;  // *length de cada vínculo; nunca nulo
    std::vector<QByteArray> dados;        // dono dos bytes que os buffers apontam
};

[[nodiscard]] std::expected<VinculosMontados, nl::base::Erro> montarVinculos(std::span<const Parametro> parametros);

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H
