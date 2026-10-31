// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Configuração da CLI (NL-2.9, DA-2.44): URL da Sefin, âncoras do A1 e do servidor TLS, XSD, liberação de Produção e algoritmo
// da assinatura (NL-2.10a). Lida do arquivo XDG (XDG_CONFIG_HOME/notalivre/config.ini). Não há argumento de servidor nem de âncoras.
#ifndef NL_CLI_CONFIGURACAO_CLI_H
#define NL_CLI_CONFIGURACAO_CLI_H

#include <QString>

#include <expected>
#include <string>

namespace nl::cli {

/// Algoritmo da assinatura da DPS, escolhido em [assinatura] algoritmo (NL-2.10a). rsa-sha1 é o padrão (DA-2.2).
enum class AlgoritmoConfigurado { RsaSha1, RsaSha256 };

/// Falha de configuração com código estável (a tabela de códigos de saída da Mid o classifica, NL-3.8) e mensagem em pt-br.
struct ErroConfiguracao
{
    std::string codigo;
    QString mensagem;
};

struct ConfiguracaoCli
{
    QString urlSefin;              ///< [sefin] url
    QString ancoras;               ///< [sefin] ancoras: arquivo PEM das âncoras (A1 e servidor TLS)
    QString xsdDiretorio;          ///< [xsd] diretorio
    QString xsdManifesto;          ///< [xsd] manifesto
    bool producaoLiberada = false; ///< [emissao] producao_liberada: nasce falso (go/no-go, Onda 12)
    AlgoritmoConfigurado algoritmoAssinatura = AlgoritmoConfigurado::RsaSha1; ///< [assinatura] algoritmo
};

/// $XDG_CONFIG_HOME/notalivre/config.ini; sem XDG_CONFIG_HOME, $HOME/.config/notalivre/config.ini.
QString caminhoConfiguracao();

/// Lê o arquivo. Chave obrigatória ausente vem com o código configuracao.chave_ausente e o nome dela na mensagem; sem arquivo,
/// o código é configuracao.arquivo_ilegivel e a mensagem diz qual caminho.
std::expected<ConfiguracaoCli, ErroConfiguracao> lerConfiguracao(const QString& caminho);

} // namespace nl::cli

#endif // NL_CLI_CONFIGURACAO_CLI_H
