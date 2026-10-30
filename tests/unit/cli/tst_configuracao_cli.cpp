// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da configuração da CLI (NL-2.9, DA-2.44): arquivo XDG, chaves obrigatórias, Produção nasce falsa.
#include "configuracao_cli.h"

#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Escreve o texto num arquivo da pasta temporária e devolve o caminho.
QString escrever(const QTemporaryDir& pasta, const QString& texto)
{
    const QString caminho = pasta.filePath(QStringLiteral("config.ini"));
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();  // sem arquivo, a leitura do teste reprova pela asserção
    }
    arquivo.write(texto.toUtf8());
    return caminho;
}

const QString kCompleta = QStringLiteral(
    "# configuração de teste\n"
    "[sefin]\n"
    "url = https://127.0.0.1:9443\n"
    "ancoras = /tmp/ancoras.pem\n"
    "[xsd]\n"
    "diretorio = /tmp/xsd\n"
    "manifesto = /tmp/xsd/MANIFESTO.sha256\n"
    "[emissao]\n"
    "producao_liberada = false\n");

} // namespace

class TstConfiguracaoCli : public QObject
{
    Q_OBJECT

private slots:
    // Arquivo completo é lido, com os valores das chaves.
    void arquivoCompletoLido();
    // Chave obrigatória ausente recusa, com o nome da chave na mensagem.
    void chaveObrigatoriaAusenteRecusada();
    // Sem a seção de emissão, Produção nasce bloqueada.
    void producaoNasceFalsaSemSecao();
    // Sem XDG_CONFIG_HOME, o caminho é o de XDG_CONFIG_HOME quando definido.
    void caminhoRespeitaXdgConfigHome();
};

void TstConfiguracaoCli::arquivoCompletoLido()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, kCompleta));
    QVERIFY2(lida.has_value(), qPrintable(lida.error()));
    QCOMPARE(lida->urlSefin, QStringLiteral("https://127.0.0.1:9443"));
    QCOMPARE(lida->ancoras, QStringLiteral("/tmp/ancoras.pem"));
    QCOMPARE(lida->xsdDiretorio, QStringLiteral("/tmp/xsd"));
    QCOMPARE(lida->xsdManifesto, QStringLiteral("/tmp/xsd/MANIFESTO.sha256"));
    QVERIFY(!lida->producaoLiberada);
}

void TstConfiguracaoCli::chaveObrigatoriaAusenteRecusada()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString semUrl = kCompleta.left(kCompleta.indexOf(QStringLiteral("url =")))
                           + kCompleta.mid(kCompleta.indexOf(QStringLiteral("ancoras =")));
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semUrl));
    QVERIFY2(!lida.has_value(), "configuração sem sefin.url tem de ser recusada");
    QVERIFY2(lida.error().contains(QStringLiteral("sefin.url")), "a mensagem tem de citar sefin.url");
}

void TstConfiguracaoCli::producaoNasceFalsaSemSecao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString semEmissao = kCompleta.left(kCompleta.indexOf(QStringLiteral("[emissao]")));
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semEmissao));
    QVERIFY2(lida.has_value(), qPrintable(lida.error()));
    QVERIFY2(!lida->producaoLiberada, "sem a seção de emissão, Produção tem de nascer bloqueada");
}

void TstConfiguracaoCli::caminhoRespeitaXdgConfigHome()
{
    qputenv("XDG_CONFIG_HOME", QByteArray("/var/tmp/xdg-teste"));
    QCOMPARE(nl::cli::caminhoConfiguracao(), QStringLiteral("/var/tmp/xdg-teste/notalivre/config.ini"));
}

QTEST_APPLESS_MAIN(TstConfiguracaoCli)
#include "tst_configuracao_cli.moc"
