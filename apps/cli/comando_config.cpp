// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Subcomando `notalivre config` (NL-2.10a, passo 6). Ver comando_config.h. Gravação atômica com QSaveFile: o config.ini nunca
// fica pela metade. O caminho do .pfx não vai à saída (pode conter nome de pessoa).
#include "comando_config.h"

#include "configuracao_cli.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSaveFile>
#include <QStringList>
#include <QTextStream>

namespace nl::cli {
namespace {

const QString kUsoConfig = QStringLiteral("uso: notalivre config certificado <caminho do .pfx>");

// Troca a chave caminho dentro de [certificado], ou a acrescenta logo após o cabeçalho da seção; sem a seção, a seção vai ao fim.
// As outras linhas e seções ficam como estão.
QString gravarCaminhoNoTexto(const QString& texto, const QString& caminho)
{
    const QString chave = QStringLiteral("caminho = %1").arg(caminho);
    QStringList linhas = texto.split(QLatin1Char('\n'));
    int cabecalho = -1;
    for (qsizetype i = 0; i < linhas.size(); ++i) {
        if (linhas.at(i).trimmed() == QLatin1String("[certificado]")) {
            cabecalho = static_cast<int>(i);
            break;
        }
    }
    if (cabecalho < 0) {
        QString novo = texto;
        if (!novo.isEmpty() && !novo.endsWith(QLatin1Char('\n'))) {
            novo += QLatin1Char('\n');
        }
        return novo + QStringLiteral("[certificado]\n") + chave + QLatin1Char('\n');
    }
    for (qsizetype i = cabecalho + 1; i < linhas.size(); ++i) {
        const QString linha = linhas.at(i).trimmed();
        if (linha.startsWith(QLatin1Char('['))) {
            break;
        }
        const qsizetype igual = linha.indexOf(QLatin1Char('='));
        if (igual > 0 && linha.left(igual).trimmed() == QLatin1String("caminho")) {
            linhas[i] = chave;
            return linhas.join(QLatin1Char('\n'));
        }
    }
    linhas.insert(cabecalho + 1, chave);
    return linhas.join(QLatin1Char('\n'));
}

} // namespace

int executarConfig(const QStringList& argumentos)
{
    QTextStream erro{stderr};
    QTextStream saida{stdout};
    if (argumentos.size() != 3 || argumentos.at(0) != QLatin1String("config") || argumentos.at(1) != QLatin1String("certificado")) {
        erro << kUsoConfig << Qt::endl;
        return 2;
    }
    const QString caminho = QFileInfo(argumentos.at(2)).absoluteFilePath();
    QFile certificado(caminho);
    if (!certificado.open(QIODevice::ReadOnly)) {
        erro << QStringLiteral("certificado.caminho_ilegivel: o arquivo do certificado não existe ou o app não consegue lê-lo. "
                               "No Flatpak, o arquivo precisa estar acessível ao app: rode flatpak document-export e use o caminho impresso.")
             << Qt::endl;
        return 10;
    }
    certificado.close();

    const QString arquivoConfig = caminhoConfiguracao();
    static_cast<void>(QDir().mkpath(QFileInfo(arquivoConfig).absolutePath()));
    QString texto;
    QFile leitura(arquivoConfig);
    if (leitura.open(QIODevice::ReadOnly | QIODevice::Text)) {
        texto = QString::fromUtf8(leitura.readAll());
    }
    QSaveFile gravacao(arquivoConfig);
    if (!gravacao.open(QIODevice::WriteOnly | QIODevice::Text)) {
        erro << QStringLiteral("não foi possível gravar a configuração em %1").arg(arquivoConfig) << Qt::endl;
        return 10;
    }
    gravacao.write(gravarCaminhoNoTexto(texto, caminho).toUtf8());
    if (!gravacao.commit()) {
        erro << QStringLiteral("não foi possível concluir a gravação da configuração") << Qt::endl;
        return 10;
    }
    saida << QStringLiteral("certificado.caminho gravado na configuração") << Qt::endl;
    return 0;
}

} // namespace nl::cli
