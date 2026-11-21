// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Montagem dos vínculos (NL-3.1a, bloco U-G): um MYSQL_BIND por Parametro, com buffer e length reais. Um átomo: só monta,
// não confere SQL nem conecta.

#include "nl/foundation/sql/vinculos.h"

#include <cstddef>

#include <utility>
#include <variant>

namespace nl::foundation::sql::detalhe {

std::expected<VinculosMontados, nl::base::Erro> montarVinculos(std::span<const Parametro> parametros)
{
    VinculosMontados montados;
    const std::size_t quantidade = parametros.size();
    montados.vinculos.assign(quantidade, MYSQL_BIND{});
    montados.tamanhos.assign(quantidade, 0UL);
    montados.nulos.assign(quantidade, static_cast<my_bool>(0));
    montados.inteiros.assign(quantidade, qint64{0});
    montados.tempos.assign(quantidade, MYSQL_TIME{});
    montados.dados.assign(quantidade, QByteArray());

    for (std::size_t i = 0; i < quantidade; ++i) {
        MYSQL_BIND& vinculo = montados.vinculos[i];
        vinculo.length = &montados.tamanhos[i];
        vinculo.is_null = &montados.nulos[i];
        const Parametro::Valor& valor = parametros[i].valor();

        if (std::holds_alternative<Nulo>(valor)) {
            vinculo.buffer_type = MYSQL_TYPE_NULL;
            montados.nulos[i] = static_cast<my_bool>(1);
        } else if (const auto* inteiro = std::get_if<qint64>(&valor)) {
            montados.inteiros[i] = *inteiro;
            vinculo.buffer_type = MYSQL_TYPE_LONGLONG;
            vinculo.buffer = &montados.inteiros[i];
            vinculo.buffer_length = sizeof(qint64);
            montados.tamanhos[i] = sizeof(qint64);
        } else if (const auto* bytesOuTexto = std::get_if<QByteArray>(&valor)) {
            montados.dados[i] = *bytesOuTexto;
            vinculo.buffer_type = parametros[i].ehBytes() ? MYSQL_TYPE_BLOB : MYSQL_TYPE_STRING;
            vinculo.buffer = montados.dados[i].data();
            vinculo.buffer_length = static_cast<unsigned long>(montados.dados[i].size());
            montados.tamanhos[i] = vinculo.buffer_length;
        } else if (const auto* dia = std::get_if<QDate>(&valor)) {
            MYSQL_TIME& tempo = montados.tempos[i];
            tempo.year = static_cast<unsigned int>(dia->year());
            tempo.month = static_cast<unsigned int>(dia->month());
            tempo.day = static_cast<unsigned int>(dia->day());
            tempo.time_type = MYSQL_TIMESTAMP_DATE;
            vinculo.buffer_type = MYSQL_TYPE_DATE;
            vinculo.buffer = &tempo;
            vinculo.buffer_length = sizeof(MYSQL_TIME);
            montados.tamanhos[i] = sizeof(MYSQL_TIME);
        } else {
            std::unreachable();
        }
    }
    return montados;
}

} // namespace nl::foundation::sql::detalhe
