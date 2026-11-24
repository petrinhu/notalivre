// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo e plano do migrador (NL-3.1b, parte b1): regra pura, sem banco. A tabela de migrações, a trava e a aplicação
// no banco são da parte b2. Mora no motor sql porque o portão de camadas só deixa um motor incluir a si mesmo e a base.

#ifndef NL_FOUNDATION_SQL_MIGRACAO_H
#define NL_FOUNDATION_SQL_MIGRACAO_H

#include "nl/base/erro.h"

#include <QByteArray>
#include <QString>

#include <expected>
#include <vector>

namespace nl::foundation::sql {

/// Arquivo de migração como vem do recurso embutido: nome no padrão V0001__descricao.sql e os bytes.
struct ArquivoMigracao {
    QString nome;
    QByteArray conteudo;
};

/// Migração conferida: versão a partir de 1, descrição tirada do nome, o SQL e o SHA-256 dos bytes (32 bytes).
struct Migracao {
    int versao = 0;
    QString descricao;
    QByteArray sql;
    QByteArray checksum;
};

/// Linha da tabela de migrações já aplicadas, como o banco a devolve (parte b2).
struct MigracaoAplicada {
    int versao = 0;
    QByteArray checksum;
};

/// Confere os nomes e devolve o catálogo em ordem de versão, de 1 a N, sem faltar nenhuma.
/// Erros: "migracao.catalogo_vazio", "migracao.nome_invalido", "migracao.versao_repetida", "migracao.versao_com_buraco".
[[nodiscard]] std::expected<std::vector<Migracao>, nl::base::Erro> catalogoDe(const std::vector<ArquivoMigracao>& arquivos);

/// Migrações que faltam aplicar, em ordem, para um catálogo vindo de catalogoDe. Nada é escrito aqui.
/// Antes de comparar com o banco, confere o próprio catálogo: sequência 1..N sem buraco e SHA-256 de cada SQL igual ao
/// checksum guardado.
/// Erros: "migracao.versao_com_buraco" (catálogo fora da sequência 1..N), "migracao.catalogo_checksum_divergente" (o SQL
/// de uma entrada do catálogo não confere com o checksum dela), "migracao.schema_mais_novo" (o banco tem versão que o
/// catálogo não conhece), "migracao.aplicadas_com_buraco" (versão faltando ou repetida no banco),
/// "migracao.checksum_divergente" (o arquivo de uma migração aplicada mudou).
[[nodiscard]] std::expected<std::vector<Migracao>, nl::base::Erro> planoDe(const std::vector<Migracao>& catalogo,
                                                                          const std::vector<MigracaoAplicada>& aplicadas);

} // namespace nl::foundation::sql

#endif // NL_FOUNDATION_SQL_MIGRACAO_H
