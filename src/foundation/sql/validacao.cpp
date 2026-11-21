// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Conferência de DadosConexao antes de qualquer socket (NL-3.1a, bloco U-G). Ordem fixa: host, porta, senha, plugins
// (proibidos, depois sha2 no aberto, depois conhecidos) e, no fixado, impressão. Nenhuma chamada do conector.

#include "nl/foundation/sql/validacao.h"

#include <QByteArray>
#include <QList>

#include <optional>
#include <string>
#include <utility>
#include <variant>

// v1b, item 2 (CISO) e v1n: exatamente dois modos de TLS, aberto e fixado. Um terceiro modo quebra a compilação aqui.
static_assert(std::variant_size_v<nl::foundation::sql::ModoTls> == 2,
              "os modos de TLS são exatamente dois: aberto e fixado (v1b, item 2)");

namespace nl::foundation::sql::detalhe {
namespace {

nl::base::Erro erro(std::string codigo, std::string mensagem)
{
    return nl::base::Erro(std::move(codigo), std::nullopt, std::move(mensagem));
}

bool impressaoValida(const QByteArray& impressao)
{
    if (impressao.size() != 64) {
        return false;
    }
    for (const char c : impressao) {
        const bool digito = c >= '0' && c <= '9';
        const bool minuscula = c >= 'a' && c <= 'f';
        const bool maiuscula = c >= 'A' && c <= 'F';
        if (!digito && !minuscula && !maiuscula) {
            return false;
        }
    }
    return true;
}

} // namespace

std::expected<void, nl::base::Erro> validarDadosConexao(const DadosConexao& dados)
{
    if (dados.host != "127.0.0.1" && dados.host != "::1") {
        return std::unexpected(erro("sql.host_fora_do_loopback", "o host tem de ser 127.0.0.1 ou ::1"));
    }
    if (dados.porta < 1 || dados.porta > 65535) {
        return std::unexpected(erro("sql.porta_invalida", "a porta tem de estar entre 1 e 65535"));
    }
    if (dados.senhaNulTerminada == nullptr) {
        return std::unexpected(erro("sql.senha_ausente", "a senha do banco não foi informada"));
    }
    if (dados.pluginsPermitidos.isEmpty()) {
        return std::unexpected(erro("sql.plugins_vazio", "a lista de plugins de autenticação está vazia"));
    }

    const bool aberto = std::holds_alternative<ModoAberto>(dados.modo);
    for (const QByteArray& plugin : dados.pluginsPermitidos.split(',')) {
        if (plugin == "mysql_clear_password" || plugin == "dialog") {
            return std::unexpected(erro("sql.plugin_proibido", "plugin de autenticação proibido em qualquer modo"));
        }
        if (aberto && (plugin == "caching_sha2_password" || plugin == "sha256_password")) {
            return std::unexpected(erro("sql.plugin_sha2_no_modo_aberto",
                                        "plugin sha2 não funciona no modo aberto (sem TLS, sem chave)"));
        }
        if (plugin != "mysql_native_password" && plugin != "caching_sha2_password" && plugin != "client_ed25519") {
            return std::unexpected(erro("sql.plugin_desconhecido", "plugin de autenticação fora da lista conhecida"));
        }
    }

    if (const auto* fixado = std::get_if<ModoFixado>(&dados.modo)) {
        if (!impressaoValida(fixado->impressaoSha256Hex)) {
            return std::unexpected(erro("sql.impressao_invalida", "a impressão do certificado tem de ter 64 dígitos hex"));
        }
    }
    return {};
}

} // namespace nl::foundation::sql::detalhe
