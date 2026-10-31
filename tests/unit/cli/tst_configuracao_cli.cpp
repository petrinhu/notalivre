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
    "prefixo = /SefinNacional\n"
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
    // [assinatura] algoritmo = rsa-sha256 chega à configuração; sem a seção, o padrão é rsa-sha1 (NL-2.10a, passo 2).
    void algoritmoRsaSha256PelaConfiguracao();
    // Algoritmo fora de rsa-sha1 e rsa-sha256 é recusado com assinatura.algoritmo_invalido, código que sai com 10 (emenda 4).
    void algoritmoForaDaListaSaiComDez();
    // NL-2.10a, passo 4: [certificado] ancoras (A1) e [sefin] ancoras_tls (servidor) são chaves separadas; sefin.ancoras sai.
    void ancorasDoA1EDoServidorSeparadas();
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
    QVERIFY2(lida.has_value(), "a configuração completa tem de ser lida");
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
    QVERIFY2(lida.error().mensagem.contains(QStringLiteral("sefin.url")), "a mensagem tem de citar sefin.url");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("configuracao.chave_ausente"));
}

void TstConfiguracaoCli::algoritmoRsaSha256PelaConfiguracao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto comSha256 = nl::cli::lerConfiguracao(escrever(pasta, kCompleta + QStringLiteral("[assinatura]\nalgoritmo = rsa-sha256\n")));
    QVERIFY2(comSha256.has_value(), "a configuração com rsa-sha256 tem de ser lida");
    QVERIFY2(comSha256->algoritmoAssinatura == nl::cli::AlgoritmoConfigurado::RsaSha256, "rsa-sha256 tem de chegar à configuração");
    // Sem a seção, o padrão é rsa-sha1 (DA-2.2): a emissão de hoje não muda.
    const auto semSecao = nl::cli::lerConfiguracao(escrever(pasta, kCompleta));
    QVERIFY2(semSecao.has_value(), "a configuração sem a seção de assinatura tem de ser lida");
    QVERIFY2(semSecao->algoritmoAssinatura == nl::cli::AlgoritmoConfigurado::RsaSha1, "sem a seção, o padrão tem de ser rsa-sha1");
}

void TstConfiguracaoCli::ancorasDoA1EDoServidorSeparadas()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString separadas = QStringLiteral(
        "[sefin]\nurl = https://127.0.0.1:9443\nprefixo = /SefinNacional\nancoras_tls = /tmp/tls.pem\n"
        "[certificado]\nancoras = /tmp/a1.pem\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, separadas));
    QVERIFY2(lida.has_value(), "as âncoras do A1 e do servidor têm de ser lidas de chaves separadas, sem sefin.ancoras");
    QCOMPARE(lida->ancorasCertificado, QStringLiteral("/tmp/a1.pem"));
    QCOMPARE(lida->ancorasTls, QStringLiteral("/tmp/tls.pem"));
}

void TstConfiguracaoCli::algoritmoForaDaListaSaiComDez()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, kCompleta + QStringLiteral("[assinatura]\nalgoritmo = md5\n")));
    QVERIFY2(!lida.has_value(), "algoritmo fora de rsa-sha1 e rsa-sha256 tem de ser recusado");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("assinatura.algoritmo_invalido"));
}

void TstConfiguracaoCli::producaoNasceFalsaSemSecao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString semEmissao = kCompleta.left(kCompleta.indexOf(QStringLiteral("[emissao]")));
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semEmissao));
    QVERIFY2(lida.has_value(), "a configuração sem a seção de emissão tem de ser lida");
    QVERIFY2(!lida->producaoLiberada, "sem a seção de emissão, Produção tem de nascer bloqueada");
}

void TstConfiguracaoCli::caminhoRespeitaXdgConfigHome()
{
    qputenv("XDG_CONFIG_HOME", QByteArray("/var/tmp/xdg-teste"));
    QCOMPARE(nl::cli::caminhoConfiguracao(), QStringLiteral("/var/tmp/xdg-teste/notalivre/config.ini"));
}

QTEST_APPLESS_MAIN(TstConfiguracaoCli)
#include "tst_configuracao_cli.moc"
