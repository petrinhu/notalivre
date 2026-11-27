// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta do GET_LOCK do migrador (NL-3.1b, parte b2; parecer do CISO, I-a). Função pura, sem chamada do conector: o
// teste de unidade cobre cada forma de resposta, inclusive a nula, que nenhum servidor de teste devolve.

#ifndef NL_FOUNDATION_SQL_PRIVADO_TRAVA_MIGRACAO_H
#define NL_FOUNDATION_SQL_PRIVADO_TRAVA_MIGRACAO_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/parametro.h"

#include <expected>

namespace nl::foundation::sql::detalhe {

/// Lê as linhas do GET_LOCK. Uma linha com uma coluna inteira 1: a trava foi dada. Inteiro 0: "migracao.trava_ocupada".
/// Qualquer outra forma (nula, sem linha, mais de uma coluna, texto, outro inteiro): "migracao.trava_falhou".
[[nodiscard]] std::expected<void, nl::base::Erro> respostaDaTrava(const Linhas& linhas);

} // namespace nl::foundation::sql::detalhe

#endif // NL_FOUNDATION_SQL_PRIVADO_TRAVA_MIGRACAO_H
