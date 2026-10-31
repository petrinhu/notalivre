// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
#include <nl/teste/tabela_por_distro.h>

#include <nl/teste/tabela_sha1_distro.h>

#include <QFile>
#include <QIODevice>
#include <QStringList>

namespace nl::teste {

namespace {

// Linhas de dados com exatamente 3 campos, já divididas. Linhas com # são ignoradas, como no leitor do SHA-1.
QList<QStringList> linhasDeDados(const QString& caminhoTabela)
{
    QFile arquivo(caminhoTabela);
    QList<QStringList> linhas;
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return linhas;
    }
    const QStringList texto = QString::fromUtf8(arquivo.readAll()).split(QLatin1Char('\n'));
    for (const QString& linha : texto) {
        if (linha.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList campos = linha.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (campos.size() == 3) {
            linhas.append(campos);
        }
    }
    return linhas;
}

} // namespace

QString valorDaDistro(const QString& caminhoTabela, const QString& id, const QString& versao)
{
    QString valor;
    for (const QStringList& campos : linhasDeDados(caminhoTabela)) {
        if (campos.at(0) == id && campos.at(1) == versao) {
            valor = campos.at(2);
        }
    }
    return valor;
}

QString valorDaDistroAtual(const QString& caminhoTabela)
{
    return valorDaDistro(caminhoTabela, campoDoOsRelease(QStringLiteral("ID")), campoDoOsRelease(QStringLiteral("VERSION_ID")));
}

QStringList valoresDaTabela(const QString& caminhoTabela)
{
    QStringList valores;
    for (const QStringList& campos : linhasDeDados(caminhoTabela)) {
        valores.append(campos.at(2));
    }
    return valores;
}

} // namespace nl::teste
