// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Montagem do array MYSQL_BIND a partir dos Parametro (NL-3.1a, bloco U-R). Dono dos buffers e dos length.

#ifndef NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H
#define NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/parametro.h"

#include <mysql.h>

#include <QByteArray>
#include <QtGlobal>

#include <expected>
#include <span>
#include <vector>

namespace nl::foundation::sql::detalhe {

// Os ponteiros de cada MYSQL_BIND apontam para os vetores abaixo. Os vetores são dimensionados uma vez, antes de
// qualquer ponteiro, e o objeto só se move (mover um std::vector mantém o bloco de memória, então os ponteiros valem).
struct VinculosMontados {
    VinculosMontados() = default;
    VinculosMontados(VinculosMontados&&) noexcept = default;
    VinculosMontados& operator=(VinculosMontados&&) noexcept = default;
    VinculosMontados(const VinculosMontados&) = delete;
    VinculosMontados& operator=(const VinculosMontados&) = delete;
    ~VinculosMontados() = default;

    std::vector<MYSQL_BIND> vinculos;     // um por parâmetro, na ordem do span
    std::vector<unsigned long> tamanhos;  // *length de cada vínculo; nunca nulo
    std::vector<my_bool> nulos;           // *is_null de cada vínculo; nunca nulo
    std::vector<qint64> inteiros;         // buffer dos inteiros
    std::vector<MYSQL_TIME> tempos;       // buffer das datas, sem fuso
    std::vector<QByteArray> dados;        // dono dos bytes de texto e de blob
};

[[nodiscard]] std::expected<VinculosMontados, nl::base::Erro> montarVinculos(std::span<const Parametro> parametros);

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_VINCULOS_H
