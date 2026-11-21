// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Política de conexão por família de servidor (NL-3.1a, bloco M-R). A família e a exigência de TLS vêm da configuração
// gravada pelo script (NL-3.6), nunca do anúncio do servidor. Mid: decide o modo; a Foundation sql só executa.

#ifndef NL_MID_PERSISTENCIA_POLITICA_CONEXAO_H
#define NL_MID_PERSISTENCIA_POLITICA_CONEXAO_H

#include "nl/base/erro.h"
#include "nl/foundation/pkcs12/senha_segura.h"
#include "nl/foundation/sql/dados_conexao.h"

#include <QByteArray>

#include <expected>

namespace nl::mid::persistencia {

enum class FamiliaServidor { Mysql, Mariadb };

struct ServidorDeclarado {                 // vem da configuração gravada pelo script (NL-3.6), nunca do anúncio do servidor
    FamiliaServidor familia;
    bool exigeTransporteSeguro = false;    // MariaDB com require_secure_transport=ON (NL-I.339): vai ao modo fixado
    QByteArray impressaoSha256Hex;         // obrigatória no fixado
    QByteArray host;
    int porta = 0;
    QByteArray banco;
    QByteArray usuario;
};

[[nodiscard]] std::expected<nl::foundation::sql::DadosConexao, nl::base::Erro>
dadosDeConexao(const ServidorDeclarado& servidor, const nl::foundation::pkcs12::SenhaSegura& senha);

} // namespace nl::mid::persistencia

#endif // NL_MID_PERSISTENCIA_POLITICA_CONEXAO_H
