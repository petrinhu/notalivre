// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Configuração da CLI (NL-2.9, DA-2.44). Ver configuracao_cli.h. Formato INI mínimo: [secao], chave = valor, # comentário.
#include "configuracao_cli.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMap>
#include <QStringList>

namespace nl::cli {
namespace {

// Valor de uma chave obrigatória, ou a mensagem com o nome completo secao.chave.
std::expected<QString, QString> obrigatoria(const QMap<QString, QString>& valores, const QString& chave)
{
    const QString valor = valores.value(chave);
    if (valor.isEmpty()) {
        return std::unexpected(QStringLiteral("configuração sem a chave obrigatória %1").arg(chave));
    }
    return valor;
}

} // namespace

QString caminhoConfiguracao()
{
    const QString xdg = qEnvironmentVariable("XDG_CONFIG_HOME");
    const QString base = xdg.isEmpty() ? QDir::homePath() + QStringLiteral("/.config") : xdg;
    return base + QStringLiteral("/notalivre/config.ini");
}

std::expected<ConfiguracaoCli, QString> lerConfiguracao(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unexpected(QStringLiteral("não foi possível ler a configuração em %1").arg(caminho));
    }
    QMap<QString, QString> valores;  // chave "secao.nome" -> valor
    QString secao;
    const QStringList linhas = QString::fromUtf8(arquivo.readAll()).split(QLatin1Char('\n'));
    for (const QString& bruta : linhas) {
        const QString linha = bruta.trimmed();
        if (linha.isEmpty() || linha.startsWith(QLatin1Char('#'))) {
            continue;
        }
        if (linha.startsWith(QLatin1Char('[')) && linha.endsWith(QLatin1Char(']'))) {
            secao = linha.mid(1, linha.size() - 2).trimmed();
            continue;
        }
        const qsizetype igual = linha.indexOf(QLatin1Char('='));
        if (igual <= 0) {
            continue;
        }
        valores.insert(secao + QLatin1Char('.') + linha.left(igual).trimmed(), linha.mid(igual + 1).trimmed());
    }

    ConfiguracaoCli cfg;
    auto url = obrigatoria(valores, QStringLiteral("sefin.url"));
    if (!url) {
        return std::unexpected(url.error());
    }
    auto ancoras = obrigatoria(valores, QStringLiteral("sefin.ancoras"));
    if (!ancoras) {
        return std::unexpected(ancoras.error());
    }
    auto diretorio = obrigatoria(valores, QStringLiteral("xsd.diretorio"));
    if (!diretorio) {
        return std::unexpected(diretorio.error());
    }
    auto manifesto = obrigatoria(valores, QStringLiteral("xsd.manifesto"));
    if (!manifesto) {
        return std::unexpected(manifesto.error());
    }
    cfg.urlSefin = *url;
    cfg.ancoras = *ancoras;
    cfg.xsdDiretorio = *diretorio;
    cfg.xsdManifesto = *manifesto;
    // Produção só liga com a chave em "true" (go/no-go do líder). Qualquer outro valor, ou ausência, fica bloqueada.
    cfg.producaoLiberada = valores.value(QStringLiteral("emissao.producao_liberada")) == QStringLiteral("true");
    return cfg;
}

} // namespace nl::cli
