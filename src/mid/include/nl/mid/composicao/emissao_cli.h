// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada do `notalivre emitir` (NL-2.9, DA-2.44): JSON -> XML validado -> Id -> credencial -> assinatura -> envio. A CLI só
// conversa com esta fachada. O adaptador de assinatura entra pela raiz EmissaoNota; o envio, pelo ClienteSefin.
#ifndef NL_MID_COMPOSICAO_EMISSAO_CLI_H
#define NL_MID_COMPOSICAO_EMISSAO_CLI_H

#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QString>

#include <expected>
#include <string>

namespace nl::foundation::log {
class Log;
}
namespace nl::foundation::pkcs12 {
class SenhaSegura;
}

namespace nl::mid::composicao {

/// Tudo o que a emissão precisa, já lido pela CLI. A senha não entra aqui: vai à parte, do terminal.
struct PedidoEmissao
{
    QByteArray dpsJson;             ///< a DPS de entrada, como JSON
    LocalizacaoEsquemaDps esquema;  ///< XSD e manifesto de Produção Restrita (config)
    QString caminhoCertificado;     ///< .pfx do certificado (argumento --certificado)
    QString urlSefin;               ///< base da API (config [sefin] url)
    QString caminhoAncoras;         ///< PEM das âncoras do A1 e do servidor TLS (config [sefin] ancoras)
};

/// Falha da emissão: o código é estável; a mensagem vem do catálogo, nunca do texto livre da API.
struct FalhaEmissao
{
    std::string codigo;
    std::string mensagem;
};

/// Emite a DPS e devolve a chave da NFS-e. Erro de entrada, de credencial ou de envio volta com código estável e sem dado de titular.
std::expected<QString, FalhaEmissao> emitirDpsPelaCli(const PedidoEmissao& pedido, const QByteArray& senha);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_EMISSAO_CLI_H
