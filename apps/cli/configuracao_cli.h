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
    QString prefixo;               ///< [sefin] prefixo: caminho da API (DA-2.5), ex.: /SefinNacional; obrigatório (NL-2.10a)
    QString ancorasCertificado;    ///< [certificado] ancoras: PEM das âncoras do A1 (NL-2.10a, passo 4)
    QString ancorasTls;            ///< [sefin] ancoras_tls: PEM das âncoras do servidor TLS (NL-2.10a, passo 4)
    QString caminhoCertificado;    ///< [certificado] caminho: o .pfx do A1 (NL-2.10a, passo 5); a senha vem do terminal
    QString xsdDiretorio;          ///< [xsd] diretorio
    QString xsdManifesto;          ///< [xsd] manifesto
    bool producaoLiberada = false; ///< [emissao] producao_liberada: nasce falso (go/no-go, Onda 12)
    AlgoritmoConfigurado algoritmoAssinatura = AlgoritmoConfigurado::RsaSha1; ///< [assinatura] algoritmo
};

/// $XDG_CONFIG_HOME/notalivre/config.ini; sem XDG_CONFIG_HOME, $HOME/.config/notalivre/config.ini.
QString caminhoConfiguracao();

/// Lê o arquivo. Chave obrigatória ausente vem com o código configuracao.chave_ausente e o nome dela na mensagem; sem arquivo,
/// o código é configuracao.arquivo_ilegivel e a mensagem diz qual caminho. dirExecutavel é o diretório do executável: sem
/// [xsd], o padrão é <dir>/../share/notalivre/xsd (NL-2.10b, bundle Flatpak em /app/bin dá /app/share/notalivre/xsd).
std::expected<ConfiguracaoCli, ErroConfiguracao> lerConfiguracao(const QString& caminho, const QString& dirExecutavel);

/// Âncoras de produção (condição do CISO, 31/10/2026, NL-2.10b): [certificado] ancoras e [sefin] ancoras_tls só valem em
/// Homologação. Fora dela, qualquer uma definida é erro de configuração (configuracao.ancoras_*_fora_de_homologacao).
std::expected<void, ErroConfiguracao> conferirAncorasDeAmbiente(const ConfiguracaoCli& cfg, bool homologacao);

} // namespace nl::cli

#endif // NL_CLI_CONFIGURACAO_CLI_H
