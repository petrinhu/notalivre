// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo do migrador (NL-3.1b, parte b1): lê a versão do nome, calcula o SHA-256 dos bytes e confere a sequência.

#include "nl/foundation/sql/migracao.h"

#include <QCryptographicHash>
#include <QRegularExpression>

#include <algorithm>
#include <optional>
#include <utility>

namespace nl::foundation::sql {
namespace {

// V0001__descricao.sql: 4 dígitos, versão a partir de 1, descrição só com minúsculas, dígitos e sublinhado.
std::optional<Migracao> migracaoDoArquivo(const ArquivoMigracao& arquivo)
{
    static const QRegularExpression padrao(QStringLiteral("^V(\\d{4})__([a-z0-9_]+)\\.sql$"));
    const QRegularExpressionMatch casamento = padrao.match(arquivo.nome);
    if (!casamento.hasMatch()) {
        return std::nullopt;
    }
    const int versao = casamento.captured(1).toInt();
    if (versao < 1) {
        return std::nullopt;
    }
    return Migracao{versao, casamento.captured(2), arquivo.conteudo,
                    QCryptographicHash::hash(arquivo.conteudo, QCryptographicHash::Sha256)};
}

} // namespace

std::expected<std::vector<Migracao>, nl::base::Erro> catalogoDe(const std::vector<ArquivoMigracao>& arquivos)
{
    if (arquivos.empty()) {
        return std::unexpected(nl::base::Erro{"migracao.catalogo_vazio", std::nullopt, "nenhum arquivo de migração"});
    }
    std::vector<Migracao> catalogo;
    catalogo.reserve(arquivos.size());
    for (const ArquivoMigracao& arquivo : arquivos) {
        std::optional<Migracao> migracao = migracaoDoArquivo(arquivo);
        if (!migracao) {
            return std::unexpected(nl::base::Erro{"migracao.nome_invalido", std::nullopt,
                                                  "nome de migração fora do padrão V0001__descricao.sql: " +
                                                      arquivo.nome.toStdString()});
        }
        catalogo.push_back(std::move(*migracao));
    }
    std::sort(catalogo.begin(), catalogo.end(),
              [](const Migracao& a, const Migracao& b) { return a.versao < b.versao; });
    for (std::size_t i = 0; i < catalogo.size(); ++i) {
        if (i > 0 && catalogo[i].versao == catalogo[i - 1].versao) {
            return std::unexpected(nl::base::Erro{"migracao.versao_repetida", std::nullopt,
                                                  "duas migrações com a mesma versão"});
        }
        if (catalogo[i].versao != static_cast<int>(i) + 1) {
            return std::unexpected(nl::base::Erro{"migracao.versao_com_buraco", std::nullopt,
                                                  "falta uma versão de migração na sequência que começa em 1"});
        }
    }
    return catalogo;
}

} // namespace nl::foundation::sql
