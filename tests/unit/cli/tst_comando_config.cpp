// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do subcomando `notalivre config certificado <caminho>` (NL-2.10a, passo 6): grava a chave no config.ini e preserva o resto.
#include "comando_config.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Lê o arquivo inteiro como texto UTF-8.
QString lerTexto(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(arquivo.readAll());
}

} // namespace

class TstComandoConfig : public QObject
{
    Q_OBJECT

private slots:
    // notalivre config certificado <caminho> confere o arquivo, grava [certificado] caminho e preserva as outras chaves.
    void configCertificadoGravaACaminho();
};

void TstComandoConfig::configCertificadoGravaACaminho()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    qputenv("XDG_CONFIG_HOME", pasta.path().toUtf8());
    QVERIFY(QDir().mkpath(pasta.filePath(QStringLiteral("notalivre"))));
    const QString pfx = pasta.filePath(QStringLiteral("a1.pfx"));
    QFile certificado(pfx);
    QVERIFY(certificado.open(QIODevice::WriteOnly));
    certificado.write("conteudo de teste, o comando só confere a leitura");
    certificado.close();
    QFile config(pasta.filePath(QStringLiteral("notalivre/config.ini")));
    QVERIFY(config.open(QIODevice::WriteOnly | QIODevice::Text));
    config.write("[sefin]\nurl = https://127.0.0.1:9443\n[xsd]\ndiretorio = /tmp/xsd\n");
    config.close();

    QCOMPARE(nl::cli::executarConfig({QStringLiteral("config"), QStringLiteral("certificado"), pfx}), 0);

    const QString texto = lerTexto(pasta.filePath(QStringLiteral("notalivre/config.ini")));
    QVERIFY2(texto.contains(QStringLiteral("[certificado]\ncaminho = %1\n").arg(pfx)), "o caminho tem de ficar em [certificado] caminho");
    QVERIFY2(texto.contains(QStringLiteral("url = https://127.0.0.1:9443")), "a chave [sefin] url tem de ser preservada");
    QVERIFY2(texto.contains(QStringLiteral("diretorio = /tmp/xsd")), "a chave [xsd] diretorio tem de ser preservada");
}

QTEST_APPLESS_MAIN(TstComandoConfig)
#include "tst_comando_config.moc"
