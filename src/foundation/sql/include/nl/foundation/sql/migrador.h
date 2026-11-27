// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Migrador no banco (NL-3.1b, parte b2): aplica o que falta do catálogo, com a trava da base, a tabela nl_migracao e o
// registro de cada versão depois do DDL dela. O SQL vem só do binário, como literal Ddl conferido na compilação: o banco
// nunca fornece SQL ao migrador (D4 do CTO, opção 1).

#ifndef NL_FOUNDATION_SQL_MIGRADOR_H
#define NL_FOUNDATION_SQL_MIGRADOR_H

#include "nl/base/erro.h"
#include "nl/foundation/sql/consulta.h"
#include "nl/foundation/sql/executor.h"

#include <expected>
#include <span>
#include <vector>

namespace nl::foundation::sql {

/// Migração embutida no binário: o nome no padrão V0001__descricao.sql e o DDL literal (sem ponto e vírgula, aspas nem
/// comentário, pela conferência do Ddl::de).
struct MigracaoEmbutida {
    const char* nome;
    Ddl ddl;
};

/// Versões aplicadas nesta chamada, em ordem. Vazio: o banco já estava na versão do catálogo.
struct ResultadoMigracao {
    std::vector<int> aplicadas;
};

/// Espera máxima pela trava da base, em segundos.
inline constexpr int kEsperaDaTravaSegundos = 5;

/// Aplica no banco as migrações que faltam, em ordem, e registra cada uma em nl_migracao depois do DDL dela.
/// Ordem: catálogo (catalogoDe), trava da base (GET_LOCK), tabela nl_migracao, leitura das aplicadas, plano (planoDe) e,
/// por versão, a marca com sucesso 0, o DDL e o sucesso 1. A trava sai em todo caminho de volta.
/// Erros: os de catalogoDe e planoDe; "migracao.trava_ocupada" (outra sessão segura a trava além da espera);
/// "migracao.trava_falhou" (o servidor não deu resposta à trava); "migracao.interrompida" (uma versão ficou com sucesso
/// 0, de um DDL que falhou ou não terminou: nada mais é aplicado); "migracao.sql_divergente" (o SQL a executar não é o
/// mesmo que gerou o checksum do catálogo); e os do Executor ("sql.*").
[[nodiscard]] std::expected<ResultadoMigracao, nl::base::Erro> migrar(Executor& executor,
                                                                     std::span<const MigracaoEmbutida> embutidas);

} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_MIGRADOR_H
