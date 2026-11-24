// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Migrador no banco (NL-3.1b, parte b2). O DDL confirma a transação anterior nos dois servidores
// (docs/pesquisa/onda3_banco_cripto.md:131-132): por isso cada versão leva uma marca antes do DDL e o sucesso depois,
// e uma marca que ficou em 0 trava as execuções seguintes.

#include "nl/foundation/sql/migrador.h"

#include "nl/foundation/sql/migracao.h"
#include "nl/foundation/sql/parametro.h"
#include "nl/foundation/sql/trava_migracao.h"

#include <QByteArray>
#include <QByteArrayView>
#include <QString>

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace nl::foundation::sql {
namespace {

// Nome da trava por base: o prefixo e 56 dígitos do SHA-256 do nome da base cabem nos 64 caracteres do GET_LOCK, e duas
// bases no mesmo servidor não disputam a mesma trava.
constexpr QByteArrayView kPrefixoDaTrava = "nl_mig_";

nl::base::Erro erroMigracao(const char* codigo, std::string mensagem)
{
    return nl::base::Erro{codigo, std::nullopt, std::move(mensagem)};
}

std::optional<qint64> inteiroDe(const Valor& valor)
{
    if (const auto* inteiro = std::get_if<qint64>(&valor)) {
        return *inteiro;
    }
    return std::nullopt;
}

// Solta a trava em todo caminho de volta de migrar, com sucesso ou com erro.
class TravaDaBase {
public:
    explicit TravaDaBase(Executor& executor) : executor_(executor) {}
    TravaDaBase(const TravaDaBase&) = delete;
    TravaDaBase& operator=(const TravaDaBase&) = delete;
    ~TravaDaBase()
    {
        if (!tomada_) {
            return;
        }
        const std::array parametros{Parametro::texto(kPrefixoDaTrava)};
        static_cast<void>(executor_.consultar(
            Consulta::de("SELECT RELEASE_LOCK(CONCAT(?, LEFT(SHA2(DATABASE(), 256), 56)))"), parametros));
    }

    std::expected<void, nl::base::Erro> tomar()
    {
        const std::array parametros{Parametro::texto(kPrefixoDaTrava), Parametro::inteiro(kEsperaDaTravaSegundos)};
        const auto linhas = executor_.consultar(
            Consulta::de("SELECT GET_LOCK(CONCAT(?, LEFT(SHA2(DATABASE(), 256), 56)), ?)"), parametros);
        if (!linhas) {
            return std::unexpected(linhas.error());
        }
        if (const auto dada = detalhe::respostaDaTrava(*linhas); !dada) {
            return std::unexpected(dada.error());
        }
        tomada_ = true;
        return {};
    }

private:
    Executor& executor_;
    bool tomada_ = false;
};

// Versões já registradas; uma marca com sucesso diferente de 1 é migração interrompida.
std::expected<std::vector<MigracaoAplicada>, nl::base::Erro> aplicadasDe(Executor& executor)
{
    const auto linhas = executor.consultar(
        Consulta::de("SELECT versao, checksum, sucesso FROM nl_migracao ORDER BY versao"), std::span<const Parametro>{});
    if (!linhas) {
        return std::unexpected(linhas.error());
    }
    std::vector<MigracaoAplicada> aplicadas;
    for (const Linha& linha : *linhas) {
        const std::optional<qint64> versao = linha.size() == 3 ? inteiroDe(linha.at(0)) : std::nullopt;
        const auto* checksum = linha.size() == 3 ? std::get_if<QByteArray>(&linha.at(1)) : nullptr;
        const std::optional<qint64> sucesso = linha.size() == 3 ? inteiroDe(linha.at(2)) : std::nullopt;
        if (!versao || checksum == nullptr || !sucesso) {
            return std::unexpected(erroMigracao("migracao.interrompida", "a tabela nl_migracao tem linha ilegível"));
        }
        if (*sucesso != 1) {
            return std::unexpected(erroMigracao(
                "migracao.interrompida", "a migração " + std::to_string(*versao) +
                                             " não terminou; restaure a cópia do banco antes de abrir esta versão"));
        }
        aplicadas.push_back(MigracaoAplicada{static_cast<int>(*versao), *checksum});
    }
    return aplicadas;
}

// O DDL embutido da versão, desde que os bytes dele sejam os mesmos que geraram o checksum do catálogo (NL-I.373).
const MigracaoEmbutida* embutidaDe(std::span<const MigracaoEmbutida> embutidas, const Migracao& migracao)
{
    for (const MigracaoEmbutida& embutida : embutidas) {
        if (QByteArray(embutida.ddl.texto().data(), embutida.ddl.texto().size()) == migracao.sql &&
            QString::fromLatin1(embutida.nome).mid(1, 4).toInt() == migracao.versao) {
            return &embutida;
        }
    }
    return nullptr;
}

std::expected<void, nl::base::Erro> aplicar(Executor& executor, const MigracaoEmbutida& embutida, const Migracao& migracao)
{
    const QByteArray descricao = migracao.descricao.toUtf8();
    const std::array marca{Parametro::inteiro(migracao.versao), Parametro::texto(descricao),
                           Parametro::bytes(migracao.checksum)};
    const auto marcada = executor.executar(
        Consulta::de("INSERT INTO nl_migracao (versao, descricao, checksum, aplicada_em, sucesso) "
                     "VALUES (?, ?, ?, UTC_TIMESTAMP(6), 0)"),
        marca);
    if (!marcada) {
        return std::unexpected(marcada.error());
    }
    const auto ddl = executor.executarDdl(embutida.ddl);
    if (!ddl) {
        return std::unexpected(ddl.error());
    }
    const std::array versao{Parametro::inteiro(migracao.versao)};
    const auto concluida = executor.executar(
        Consulta::de("UPDATE nl_migracao SET sucesso = 1, aplicada_em = UTC_TIMESTAMP(6) WHERE versao = ?"), versao);
    if (!concluida) {
        return std::unexpected(concluida.error());
    }
    return {};
}

} // namespace

std::expected<ResultadoMigracao, nl::base::Erro> migrar(Executor& executor, std::span<const MigracaoEmbutida> embutidas)
{
    std::vector<ArquivoMigracao> arquivos;
    arquivos.reserve(embutidas.size());
    for (const MigracaoEmbutida& embutida : embutidas) {
        arquivos.push_back(ArquivoMigracao{QString::fromLatin1(embutida.nome),
                                           QByteArray(embutida.ddl.texto().data(), embutida.ddl.texto().size())});
    }
    const auto catalogo = catalogoDe(arquivos);
    if (!catalogo) {
        return std::unexpected(catalogo.error());
    }

    TravaDaBase trava(executor);
    if (const auto tomada = trava.tomar(); !tomada) {
        return std::unexpected(tomada.error());
    }
    const auto tabela = executor.executarDdl(
        Ddl::de("CREATE TABLE IF NOT EXISTS nl_migracao (versao INT NOT NULL PRIMARY KEY, descricao VARCHAR(255) NOT NULL, "
                "checksum BINARY(32) NOT NULL, aplicada_em DATETIME(6) NOT NULL, sucesso TINYINT NOT NULL) "
                "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin"));
    if (!tabela) {
        return std::unexpected(tabela.error());
    }
    const auto aplicadas = aplicadasDe(executor);
    if (!aplicadas) {
        return std::unexpected(aplicadas.error());
    }
    const auto plano = planoDe(*catalogo, *aplicadas);
    if (!plano) {
        return std::unexpected(plano.error());
    }

    ResultadoMigracao resultado;
    for (const Migracao& migracao : *plano) {
        const MigracaoEmbutida* embutida = embutidaDe(embutidas, migracao);
        if (embutida == nullptr) {
            return std::unexpected(erroMigracao("migracao.sql_divergente",
                                                "a migração " + std::to_string(migracao.versao) +
                                                    " não tem o mesmo SQL que gerou o checksum do catálogo"));
        }
        if (const auto aplicada = aplicar(executor, *embutida, migracao); !aplicada) {
            return std::unexpected(aplicada.error());
        }
        resultado.aplicadas.push_back(migracao.versao);
    }
    return resultado;
}

} // namespace nl::foundation::sql
