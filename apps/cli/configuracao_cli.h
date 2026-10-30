// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Configuração da CLI (NL-2.9, DA-2.44): URL da Sefin, âncoras do A1 e do servidor TLS, XSD, e liberação de Produção. Lida
// do arquivo XDG (XDG_CONFIG_HOME/notalivre/config.ini). Não há argumento de servidor nem de âncoras.
#ifndef NL_CLI_CONFIGURACAO_CLI_H
#define NL_CLI_CONFIGURACAO_CLI_H

#include <QString>

#include <expected>

namespace nl::cli {

struct ConfiguracaoCli
{
    QString urlSefin;              ///< [sefin] url
    QString ancoras;               ///< [sefin] ancoras: arquivo PEM das âncoras (A1 e servidor TLS)
    QString xsdDiretorio;          ///< [xsd] diretorio
    QString xsdManifesto;          ///< [xsd] manifesto
    bool producaoLiberada = false; ///< [emissao] producao_liberada: nasce falso (go/no-go, Onda 12)
};

/// $XDG_CONFIG_HOME/notalivre/config.ini; sem XDG_CONFIG_HOME, $HOME/.config/notalivre/config.ini.
QString caminhoConfiguracao();

/// Lê o arquivo. Chave obrigatória ausente vem com o nome dela na mensagem; sem arquivo, a mensagem diz qual caminho.
std::expected<ConfiguracaoCli, QString> lerConfiguracao(const QString& caminho);

} // namespace nl::cli

#endif // NL_CLI_CONFIGURACAO_CLI_H
