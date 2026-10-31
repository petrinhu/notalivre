// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada do `notalivre emitir` (NL-2.9, DA-2.44): JSON -> XML validado -> Id -> credencial -> assinatura -> envio. A CLI só
// conversa com esta fachada. O adaptador de assinatura entra pela raiz EmissaoNota; o envio, pelo ClienteSefin.
#ifndef NL_MID_COMPOSICAO_EMISSAO_CLI_H
#define NL_MID_COMPOSICAO_EMISSAO_CLI_H

#include <nl/mid/composicao/dps_validar.h>
#include <nl/mid/composicao/politica_emissao.h>

#include <optional>
#include <QByteArray>
#include <QList>
#include <QString>

#include <chrono>
#include <expected>
#include <functional>
#include <memory>
#include <string>

namespace nl::foundation::pkcs12 {
class Ancoras;
class SenhaSegura;
class CredencialX509;
}
// Forward: a CLI inclui este cabeçalho sem QtNetwork (o nl_mid liga Qt6::Network como PRIVATE). Quem chama as funções inclui o tipo.
class QSslCertificate;
namespace nl::back {
class AssinadorDps;
}

namespace nl::mid::composicao {

class RegistroCli;

/// Algoritmo da assinatura da DPS, escolhido na configuração pela CLI (NL-2.10a). Enum próprio da fachada: a CLI não inclui a Mid
/// sefin. A fachada converte para o enum do adaptador (src/mid/composicao, DA-2.42).
enum class AlgoritmoAssinatura { RsaSha1, RsaSha256 };

/// Tudo o que a emissão precisa, já lido pela CLI. A senha não entra aqui: vai à parte, do terminal.
struct PedidoEmissao
{
    QByteArray dpsJson;             ///< a DPS de entrada, como JSON
    LocalizacaoEsquemaDps esquema;  ///< XSD e manifesto de Produção Restrita (config)
    QString caminhoCertificado;     ///< .pfx do certificado (argumento --certificado)
    QString urlSefin;               ///< base da API (config [sefin] url)
    QString caminhoAncoras;         ///< PEM das âncoras do A1 (config [certificado] ancoras, NL-2.10a)
    QString confirmacao;            ///< CPF ou CNPJ digitado no terminal; vazio se não foi digitado (DA-2.14: em Produção, obrigatório)
    AlgoritmoAssinatura algoritmo = AlgoritmoAssinatura::RsaSha1;  ///< [assinatura] algoritmo (NL-2.10a); padrão rsa-sha1
    QString prefixoSefin = QString();  ///< prefixo do caminho da API (config [sefin] prefixo, DA-2.5), ex.: /SefinNacional; a CLI sempre o preenche
    QString caminhoAncorasTls = QString();  ///< PEM das âncoras do servidor TLS (config [sefin] ancoras_tls, NL-2.10a); a CLI sempre o preenche
    std::optional<Ambiente> ambiente = std::nullopt;  ///< ambiente que o chamador afirma (NL-2.10b, MÉDIA-1): sem valor padrão; a Mid deriva de urlSefin
    QString diretorioRaizesIcp = QString();     ///< diretório das raízes ICP do bundle, <prefixo>/share/notalivre/icp-brasil (NL-2.10b, emenda 5)
};

/// Falha da emissão: o código é estável; a mensagem vem do catálogo, nunca do texto livre da API.
struct FalhaEmissao
{
    std::string codigo;
    std::string mensagem;
};

/// Âncoras do A1 para a emissão (NL-2.10b, emenda 5). Em Homologação, o PEM de caminhoAncoras; fora dela, sempre as raízes ICP
/// embutidas (carregarRaizesDeProducao em diretorioRaizesIcp), e caminhoAncoras é ignorado.
std::expected<nl::foundation::pkcs12::Ancoras, FalhaEmissao> ancorasDoA1(const PedidoEmissao& pedido,
                                                                          std::chrono::system_clock::time_point agora);

/// Âncoras do servidor TLS (NL-2.10b, emenda 4): o PEM de caminhoAncorasTls; sem ele, as CAs do sistema do runtime.
std::expected<QList<QSslCertificate>, FalhaEmissao> ancorasTlsDoPedido(const PedidoEmissao& pedido);

/// Emite a DPS e devolve a chave da NFS-e. Erro de entrada, de credencial ou de envio volta com código estável e sem dado de titular.
/// Fábrica do assinador da DPS, dada a credencial já carregada. A emissão real usa AssinadorDpsSefin; o teste injeta outro
/// (DA-2.56). É o port AssinadorDps, não um seam novo.
using FabricaAssinador = std::function<std::unique_ptr<nl::back::AssinadorDps>(const nl::foundation::pkcs12::CredencialX509&)>;

/// A senha é consumida: o carregador a zera (NL-2.9, item 3). O registro recebe o evento da emissão (DA-2.54).
std::expected<QString, FalhaEmissao> emitirDpsPelaCli(const PedidoEmissao& pedido, nl::foundation::pkcs12::SenhaSegura&& senha,
                                                      RegistroCli& registro);

/// Lê a confirmação (CPF ou CNPJ) no terminal sem eco. A CLI usa só esta fachada, não o cofre (matriz 1.1, DA-2.59).
std::optional<QString> lerConfirmacaoPeloTerminal(const QString& prompt);

/// Lê a senha do terminal sem eco e emite (NL-2.9, item 3): a senha nunca sai desta fachada. Sem terminal de controle, sai com
/// cofre.senha_ausente, que a CLI traduz para o código 2.
std::expected<QString, FalhaEmissao> emitirDpsPeloTerminal(const PedidoEmissao& pedido, RegistroCli& registro,
                                                          const QString& promptSenha);

/// Mesma emissão, com o assinador vindo da fábrica (DA-2.56: a revalidação depois de assinar é testada com um assinador falso).
std::expected<QString, FalhaEmissao> emitirDpsComAssinador(const PedidoEmissao& pedido, nl::foundation::pkcs12::SenhaSegura&& senha,
                                                           RegistroCli& registro, const FabricaAssinador& fabrica);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_EMISSAO_CLI_H
