// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada do `notalivre emitir` (NL-2.9, DA-2.44): JSON -> XML validado -> Id -> credencial -> assinatura -> envio. A CLI só
// conversa com esta fachada. O adaptador de assinatura entra pela raiz EmissaoNota; o envio, pelo ClienteSefin.
#ifndef NL_MID_COMPOSICAO_EMISSAO_CLI_H
#define NL_MID_COMPOSICAO_EMISSAO_CLI_H

#include <nl/back/ports/assinador_dps.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/mid/composicao/dps_validar.h>

#include <QByteArray>
#include <QString>

#include <expected>
#include <functional>
#include <memory>
#include <string>

namespace nl::foundation::pkcs12 {
class SenhaSegura;
}

namespace nl::mid::composicao {

class RegistroCli;

/// Tudo o que a emissão precisa, já lido pela CLI. A senha não entra aqui: vai à parte, do terminal.
struct PedidoEmissao
{
    QByteArray dpsJson;             ///< a DPS de entrada, como JSON
    LocalizacaoEsquemaDps esquema;  ///< XSD e manifesto de Produção Restrita (config)
    QString caminhoCertificado;     ///< .pfx do certificado (argumento --certificado)
    QString urlSefin;               ///< base da API (config [sefin] url)
    QString caminhoAncoras;         ///< PEM das âncoras do A1 e do servidor TLS (config [sefin] ancoras)
    QString confirmacao;            ///< CPF ou CNPJ digitado no terminal; vazio se não foi digitado (DA-2.14: em Produção, obrigatório)
};

/// Falha da emissão: o código é estável; a mensagem vem do catálogo, nunca do texto livre da API.
struct FalhaEmissao
{
    std::string codigo;
    std::string mensagem;
};

/// Emite a DPS e devolve a chave da NFS-e. Erro de entrada, de credencial ou de envio volta com código estável e sem dado de titular.
/// Fábrica do assinador da DPS, dada a credencial já carregada. A emissão real usa AssinadorDpsSefin; o teste injeta outro
/// (DA-2.56). É o port AssinadorDps, não um seam novo.
using FabricaAssinador = std::function<std::unique_ptr<nl::back::AssinadorDps>(const nl::foundation::pkcs12::CredencialX509&)>;

/// A senha é consumida: o carregador a zera (NL-2.9, item 3). O registro recebe o evento da emissão (DA-2.54).
std::expected<QString, FalhaEmissao> emitirDpsPelaCli(const PedidoEmissao& pedido, nl::foundation::pkcs12::SenhaSegura& senha,
                                                      RegistroCli& registro);

/// Mesma emissão, com o assinador vindo da fábrica (DA-2.56: a revalidação depois de assinar é testada com um assinador falso).
std::expected<QString, FalhaEmissao> emitirDpsComAssinador(const PedidoEmissao& pedido, nl::foundation::pkcs12::SenhaSegura& senha,
                                                           RegistroCli& registro, const FabricaAssinador& fabrica);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_EMISSAO_CLI_H
