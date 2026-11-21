// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de conexão por família de servidor (NL-3.1a, bloco M-G). Regras (ordem v1, seção 1.4): MySQL vai ao modo
// fixado com caching_sha2_password e exige impressão; MariaDB sem exigência vai ao modo aberto com mysql_native_password;
// MariaDB com exigência de transporte seguro vai ao modo fixado com mysql_native_password. A senha não é copiada.

#include "nl/mid/persistencia/politica_conexao.h"

#include <optional>
#include <utility>

namespace nl::mid::persistencia {

std::expected<nl::foundation::sql::DadosConexao, nl::base::Erro>
dadosDeConexao(const ServidorDeclarado& servidor, const nl::foundation::pkcs12::SenhaSegura& senha)
{
    namespace sql = nl::foundation::sql;
    const bool fixado = servidor.familia == FamiliaServidor::Mysql || servidor.exigeTransporteSeguro;
    if (fixado && servidor.impressaoSha256Hex.isEmpty()) {
        return std::unexpected(nl::base::Erro("persistencia.impressao_ausente", std::nullopt,
                                              "o modo fixado exige a impressão SHA-256 do certificado do servidor"));
    }

    sql::DadosConexao dados{
        .host = servidor.host,
        .porta = servidor.porta,
        .banco = servidor.banco,
        .usuario = servidor.usuario,
        .senhaNulTerminada = senha.comoC(),
        .pluginsPermitidos = servidor.familia == FamiliaServidor::Mysql ? QByteArray("caching_sha2_password")
                                                                       : QByteArray("mysql_native_password"),
        .modo = fixado ? sql::ModoTls(sql::ModoFixado(servidor.impressaoSha256Hex))
                       : sql::ModoTls(sql::ModoAberto{}),
    };
    return dados;
}

} // namespace nl::mid::persistencia
