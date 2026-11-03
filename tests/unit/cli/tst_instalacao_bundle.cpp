// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Instalação do bundle (NL-2.10b, passo 1; emenda 1 do CTO): `cmake --install` do build para um prefixo temporário, com
// o SHA-256 de cada arquivo conferido contra o SHA256SUMS de origem. O prefixo é temporário: o teste não escreve em /app.
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Uma linha "<sha256>  <nome>" do formato do sha256sum. Comentários do SHA256SUMS não entram.
struct EntradaSha
{
    QString sha;
    QString nome;
};

QList<EntradaSha> lerSha256Sums(const QString& caminho)
{
    QList<EntradaSha> entradas;
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return entradas;
    }
    const QRegularExpression linha(QStringLiteral("^([0-9a-f]{64})  ([A-Za-z0-9_.-]+)$"));
    while (!arquivo.atEnd()) {
        const QRegularExpressionMatch casa = linha.match(QString::fromUtf8(arquivo.readLine()).trimmed());
        if (casa.hasMatch()) {
            entradas.append(EntradaSha{casa.captured(1), casa.captured(2)});
        }
    }
    return entradas;
}

QString sha256DoArquivo(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromLatin1(QCryptographicHash::hash(arquivo.readAll(), QCryptographicHash::Sha256).toHex());
}

// Roda `cmake --install` do build para o prefixo. Devolve vazio se deu certo, ou o motivo da falha.
QString instalarNoPrefixo(const QString& prefixo)
{
    QProcess processo;
    processo.start(QStringLiteral(NL_CMAKE), {QStringLiteral("--install"), QStringLiteral(NL_DIRETORIO_BUILD),
                                              QStringLiteral("--prefix"), prefixo});
    if (!processo.waitForFinished(600000)) {
        return QStringLiteral("cmake --install não terminou em 10 minutos");
    }
    if (processo.exitStatus() != QProcess::NormalExit || processo.exitCode() != 0) {
        return QStringLiteral("cmake --install saiu com %1: %2")
            .arg(processo.exitCode())
            .arg(QString::fromUtf8(processo.readAllStandardError()));
    }
    return QString();
}

// Cada arquivo do SHA256SUMS de origem tem de estar instalado em `pasta` com o mesmo SHA-256, e o próprio SHA256SUMS também.
void conferirConjunto(const QString& pasta, const QString& manifestoDeOrigem)
{
    const QList<EntradaSha> entradas = lerSha256Sums(manifestoDeOrigem);
    QVERIFY2(!entradas.isEmpty(), qPrintable("SHA256SUMS de origem sem linhas de hash: " + manifestoDeOrigem));
    QVERIFY2(QFileInfo::exists(pasta + QStringLiteral("/SHA256SUMS")), qPrintable("SHA256SUMS não instalado em " + pasta));
    for (const EntradaSha& entrada : entradas) {
        const QString instalado = pasta + QLatin1Char('/') + entrada.nome;
        QVERIFY2(QFileInfo::exists(instalado), qPrintable("arquivo não instalado: " + instalado));
        QCOMPARE(sha256DoArquivo(instalado), entrada.sha);
    }
}

} // namespace

class TstInstalacaoBundle : public QObject
{
    Q_OBJECT

private slots:
    void raizesIcpInstaladasNoBundle();
    void xsdInstaladosNoBundle();
};

void TstInstalacaoBundle::raizesIcpInstaladasNoBundle()
{
    QTemporaryDir prefixo;
    QVERIFY(prefixo.isValid());
    const QString falha = instalarNoPrefixo(prefixo.path());
    QVERIFY2(falha.isEmpty(), qPrintable(falha));
    conferirConjunto(prefixo.path() + QStringLiteral("/share/notalivre/icp-brasil"), QStringLiteral(NL_ICP_MANIFESTO));
}

void TstInstalacaoBundle::xsdInstaladosNoBundle()
{
    QTemporaryDir prefixo;
    QVERIFY(prefixo.isValid());
    const QString falha = instalarNoPrefixo(prefixo.path());
    QVERIFY2(falha.isEmpty(), qPrintable(falha));
    conferirConjunto(prefixo.path() + QStringLiteral("/share/notalivre/xsd"), QStringLiteral(NL_XSD_MANIFESTO));
    QVERIFY2(QFileInfo::exists(prefixo.path() + QStringLiteral("/bin/notalivre-cli")), "binário da CLI não instalado em bin");
    QVERIFY2(QFileInfo::exists(prefixo.path() + QStringLiteral("/bin/notalivre-gui")), "binário da GUI não instalado em bin");
}

QTEST_APPLESS_MAIN(TstInstalacaoBundle)
#include "tst_instalacao_bundle.moc"
