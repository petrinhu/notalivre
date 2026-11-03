// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#include <nl/teste/tabela_sha1_distro.h>

#include <QFile>
#include <QIODevice>
#include <QStringList>

namespace nl::teste {

namespace {

QString lerTexto(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(arquivo.readAll());
}

} // namespace

QString campoDoOsRelease(const QString& campo)
{
    for (const QString& linha : lerTexto(QStringLiteral("/etc/os-release")).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        if (linha.startsWith(campo + QLatin1Char('='))) {
            return linha.mid(campo.size() + 1).remove(QLatin1Char('"'));
        }
    }
    return campo == QStringLiteral("VERSION_ID") ? QStringLiteral("rolling") : QString();
}

QString expectativaSha1(const QString& caminhoTabela, const QString& id, const QString& versao)
{
    // A última linha que casa vence. Linhas com # ou com número de campos diferente de 3 são ignoradas.
    QString esperado;
    for (const QString& linha : lerTexto(caminhoTabela).split(QLatin1Char('\n'))) {
        if (linha.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList campos = linha.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (campos.size() == 3 && campos.at(0) == id && campos.at(1) == versao) {
            esperado = campos.at(2);
        }
    }
    if (esperado == QStringLiteral("sha1") || esperado == QStringLiteral("algoritmo_bloqueado")) {
        return esperado;
    }
    return {};
}

QString expectativaSha1DaDistroAtual(const QString& caminhoTabela)
{
    return expectativaSha1(caminhoTabela, campoDoOsRelease(QStringLiteral("ID")), campoDoOsRelease(QStringLiteral("VERSION_ID")));
}

} // namespace nl::teste
