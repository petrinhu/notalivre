// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da configuração da CLI (NL-2.9, DA-2.44): arquivo XDG, chaves obrigatórias, Produção nasce falsa.
// NL-2.10b: [xsd] tem padrão do bundle, e as âncoras só valem em Homologação (condição do CISO, 31/10/2026).
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

// Diretório do executável que os testes fingem: o bundle instalado em /app/bin (NL-2.10b).
const QString kDirExecutavel = QStringLiteral("/app/bin");

const QString kCompleta = QStringLiteral(
    "# configuração de teste\n"
    "[sefin]\n"
    "url = https://127.0.0.1:9443\n"
    "prefixo = /SefinNacional\n"
    "ancoras_tls = /tmp/tls.pem\n"
    "[certificado]\n"
    "ancoras = /tmp/ancoras.pem\n"
    "caminho = /tmp/cert.pfx\n"
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
    // Chave obrigatória ausente (sefin.url) é recusada com o código e o nome da chave.
    void chaveObrigatoriaAusenteRecusada();
    // Algoritmo rsa-sha256 vem da configuração; sem a seção, o padrão é rsa-sha1.
    void algoritmoRsaSha256PelaConfiguracao();
    // Algoritmo fora da lista sai com 10.
    void algoritmoForaDaListaSaiComDez();
    // As âncoras do A1 e do servidor TLS são chaves separadas (NL-2.10a).
    void ancorasDoA1EDoServidorSeparadas();
    // Sem [emissao], Produção nasce bloqueada.
    void producaoNasceFalsaSemSecao();
    // XDG_CONFIG_HOME muda o caminho da configuração.
    void caminhoRespeitaXdgConfigHome();
    // NL-2.10b, passo 2: sem [xsd], a CLI usa <prefixo>/share/notalivre/xsd, achado a partir do executável.
    void xsdPadraoDoBundle();
    // NL-2.10b, condição do CISO: certificado.ancoras definida fora de Homologação é erro de configuração, código 10.
    void ancorasDoA1PorConfiguracaoRecusadasForaDeHomologacao();
    // NL-2.10b, condição do CISO: sefin.ancoras_tls definida fora de Homologação é erro de configuração, código 10.
    void ancorasTlsPorConfiguracaoRecusadasForaDeHomologacao();
    // NL-2.10b, BAIXA-5: chave repetida é recusada, sem escolher a última ocorrência.
    void chaveDuplicadaRecusada();
    // NL-2.10b, BAIXA-5: chave de âncora presente e vazia não contorna a guarda de ambiente: a leitura recusa.
    void chaveVaziaNaoContornaAGuarda();
    // NL-2.10b, BAIXA-4: [xsd] definido fora de Homologação é erro de configuração, código 10.
    void xsdPorConfiguracaoRecusadoForaDeHomologacao();
    // NL-2.10b, A2: o diretório padrão das raízes ICP sai de <dir do executável>/../share/notalivre/icp-brasil.
    void diretorioPadraoDasRaizesIcp();
};

void TstConfiguracaoCli::arquivoCompletoLido()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, kCompleta), kDirExecutavel);
    QVERIFY2(lida.has_value(), "a configuração completa tem de ser lida");
    QCOMPARE(lida->urlSefin, QStringLiteral("https://127.0.0.1:9443"));
    QCOMPARE(lida->ancorasCertificado, QStringLiteral("/tmp/ancoras.pem"));
    QCOMPARE(lida->caminhoCertificado, QStringLiteral("/tmp/cert.pfx"));
    QCOMPARE(lida->ancorasTls, QStringLiteral("/tmp/tls.pem"));
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
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semUrl), kDirExecutavel);
    QVERIFY2(!lida.has_value(), "configuração sem sefin.url tem de ser recusada");
    QVERIFY2(lida.error().mensagem.contains(QStringLiteral("sefin.url")), "a mensagem tem de citar sefin.url");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("configuracao.chave_ausente"));
}

void TstConfiguracaoCli::algoritmoRsaSha256PelaConfiguracao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto comSha256 = nl::cli::lerConfiguracao(escrever(pasta, kCompleta + QStringLiteral("[assinatura]\nalgoritmo = rsa-sha256\n")), kDirExecutavel);
    QVERIFY2(comSha256.has_value(), "a configuração com rsa-sha256 tem de ser lida");
    QVERIFY2(comSha256->algoritmoAssinatura == nl::cli::AlgoritmoConfigurado::RsaSha256, "rsa-sha256 tem de chegar à configuração");
    // Sem a seção, o padrão é rsa-sha1 (DA-2.2): a emissão de hoje não muda.
    const auto semSecao = nl::cli::lerConfiguracao(escrever(pasta, kCompleta), kDirExecutavel);
    QVERIFY2(semSecao.has_value(), "a configuração sem a seção de assinatura tem de ser lida");
    QVERIFY2(semSecao->algoritmoAssinatura == nl::cli::AlgoritmoConfigurado::RsaSha1, "sem a seção, o padrão tem de ser rsa-sha1");
}

void TstConfiguracaoCli::ancorasDoA1EDoServidorSeparadas()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString separadas = QStringLiteral(
        "[sefin]\nurl = https://127.0.0.1:9443\nprefixo = /SefinNacional\nancoras_tls = /tmp/tls.pem\n"
        "[certificado]\nancoras = /tmp/a1.pem\ncaminho = /tmp/a1.pfx\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, separadas), kDirExecutavel);
    QVERIFY2(lida.has_value(), "as âncoras do A1 e do servidor têm de ser lidas de chaves separadas, sem sefin.ancoras");
    QCOMPARE(lida->ancorasCertificado, QStringLiteral("/tmp/a1.pem"));
    QCOMPARE(lida->ancorasTls, QStringLiteral("/tmp/tls.pem"));
    QCOMPARE(lida->caminhoCertificado, QStringLiteral("/tmp/a1.pfx"));
}

void TstConfiguracaoCli::algoritmoForaDaListaSaiComDez()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, kCompleta + QStringLiteral("[assinatura]\nalgoritmo = md5\n")), kDirExecutavel);
    QVERIFY2(!lida.has_value(), "algoritmo fora de rsa-sha1 e rsa-sha256 tem de ser recusado");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("assinatura.algoritmo_invalido"));
}

void TstConfiguracaoCli::producaoNasceFalsaSemSecao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString semEmissao = kCompleta.left(kCompleta.indexOf(QStringLiteral("[emissao]")));
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semEmissao), kDirExecutavel);
    QVERIFY2(lida.has_value(), "a configuração sem a seção de emissão tem de ser lida");
    QVERIFY2(!lida->producaoLiberada, "sem a seção de emissão, Produção tem de nascer bloqueada");
}

void TstConfiguracaoCli::caminhoRespeitaXdgConfigHome()
{
    qputenv("XDG_CONFIG_HOME", QByteArray("/var/tmp/xdg-teste"));
    QCOMPARE(nl::cli::caminhoConfiguracao(), QStringLiteral("/var/tmp/xdg-teste/notalivre/config.ini"));
}

void TstConfiguracaoCli::xsdPadraoDoBundle()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // Sem as chaves [xsd]: o padrão sai de <dir do executável>/../share/notalivre/xsd, que no Flatpak é /app/share/notalivre/xsd.
    const QString semXsd = kCompleta.left(kCompleta.indexOf(QStringLiteral("[xsd]")))
                           + kCompleta.mid(kCompleta.indexOf(QStringLiteral("[emissao]")));
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, semXsd), kDirExecutavel);
    QVERIFY2(lida.has_value(), "sem [xsd], a configuração tem de ser lida e o padrão do bundle vale");
    QCOMPARE(lida->xsdDiretorio, QStringLiteral("/app/share/notalivre/xsd"));
    QCOMPARE(lida->xsdManifesto, QStringLiteral("/app/share/notalivre/xsd/SHA256SUMS"));
}

void TstConfiguracaoCli::ancorasDoA1PorConfiguracaoRecusadasForaDeHomologacao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // Só [certificado] ancoras definida: sem ancoras_tls, o erro tem de ser o do A1.
    const QString soA1 = QStringLiteral(
        "[sefin]\nurl = https://sefin.producaorestrita.nfse.gov.br/SefinNacional\nprefixo = /SefinNacional\n"
        "[certificado]\nancoras = /tmp/a1.pem\ncaminho = /tmp/a1.pfx\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, soA1), kDirExecutavel);
    QVERIFY2(lida.has_value(), "a configuração com âncora do A1 tem de ser lida; a recusa é da conferência por ambiente");
    const auto foraDeHomologacao = nl::cli::conferirAncorasDeAmbiente(*lida, false);
    QVERIFY2(!foraDeHomologacao.has_value(), "âncora do A1 definida fora de Homologação tem de ser recusada");
    QCOMPARE(QString::fromStdString(foraDeHomologacao.error().codigo), QStringLiteral("configuracao.ancoras_a1_fora_de_homologacao"));
    QVERIFY2(nl::cli::conferirAncorasDeAmbiente(*lida, true).has_value(), "em Homologação, a âncora do A1 é aceita");
}

void TstConfiguracaoCli::ancorasTlsPorConfiguracaoRecusadasForaDeHomologacao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // Só [sefin] ancoras_tls definida: sem certificado.ancoras, o erro tem de ser o do servidor TLS.
    const QString soTls = QStringLiteral(
        "[sefin]\nurl = https://sefin.producaorestrita.nfse.gov.br/SefinNacional\nprefixo = /SefinNacional\nancoras_tls = /tmp/tls.pem\n"
        "[certificado]\ncaminho = /tmp/a1.pfx\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, soTls), kDirExecutavel);
    QVERIFY2(lida.has_value(), "a configuração com âncora TLS tem de ser lida; a recusa é da conferência por ambiente");
    const auto foraDeHomologacao = nl::cli::conferirAncorasDeAmbiente(*lida, false);
    QVERIFY2(!foraDeHomologacao.has_value(), "âncora TLS definida fora de Homologação tem de ser recusada");
    QCOMPARE(QString::fromStdString(foraDeHomologacao.error().codigo), QStringLiteral("configuracao.ancoras_tls_fora_de_homologacao"));
    QVERIFY2(nl::cli::conferirAncorasDeAmbiente(*lida, true).has_value(), "em Homologação, a âncora TLS é aceita");
}

void TstConfiguracaoCli::chaveDuplicadaRecusada()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // A segunda [sefin] url repete sefin.url: a leitura tem de recusar, e não escolher uma das duas.
    const QString duplicada = kCompleta + QStringLiteral("[sefin]\nurl = https://sefin.nfse.gov.br/SefinNacional\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, duplicada), kDirExecutavel);
    QVERIFY2(!lida.has_value(), "chave repetida tem de ser recusada, sem escolher uma das ocorrências");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("configuracao.chave_duplicada"));
}

void TstConfiguracaoCli::chaveVaziaNaoContornaAGuarda()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    // Produção com [certificado] ancoras presente e vazia: a guarda olha a presença da chave, então o valor vazio não pode passar.
    const QString vazia = QStringLiteral(
        "[sefin]\nurl = https://sefin.nfse.gov.br/SefinNacional\nprefixo = /SefinNacional\n"
        "[certificado]\nancoras =\ncaminho = /tmp/a1.pfx\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, vazia), kDirExecutavel);
    QVERIFY2(!lida.has_value(), "âncora presente e vazia tem de ser recusada: a guarda não pode ser contornada por valor vazio");
    QCOMPARE(QString::fromStdString(lida.error().codigo), QStringLiteral("configuracao.chave_vazia"));
}

void TstConfiguracaoCli::xsdPorConfiguracaoRecusadoForaDeHomologacao()
{
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    const QString prodComXsd = QStringLiteral(
        "[sefin]\nurl = https://sefin.producaorestrita.nfse.gov.br/SefinNacional\nprefixo = /SefinNacional\n"
        "[certificado]\ncaminho = /tmp/a1.pfx\n"
        "[xsd]\ndiretorio = /tmp/xsd\nmanifesto = /tmp/xsd/MANIFESTO.sha256\n"
        "[emissao]\nproducao_liberada = false\n");
    const auto lida = nl::cli::lerConfiguracao(escrever(pasta, prodComXsd), kDirExecutavel);
    QVERIFY2(lida.has_value(), "a configuração com [xsd] tem de ser lida; a recusa é da conferência por ambiente");
    const auto foraDeHomologacao = nl::cli::conferirXsdDeAmbiente(*lida, false);
    QVERIFY2(!foraDeHomologacao.has_value(), "[xsd] definido fora de Homologação tem de ser recusado");
    QCOMPARE(QString::fromStdString(foraDeHomologacao.error().codigo), QStringLiteral("configuracao.xsd_fora_de_homologacao"));
    QVERIFY2(nl::cli::conferirXsdDeAmbiente(*lida, true).has_value(), "em Homologação, [xsd] é aceito");
}

void TstConfiguracaoCli::diretorioPadraoDasRaizesIcp()
{
    QCOMPARE(nl::cli::diretorioPadraoDasRaizesIcp(kDirExecutavel), QStringLiteral("/app/share/notalivre/icp-brasil"));
}

QTEST_APPLESS_MAIN(TstConfiguracaoCli)
#include "tst_configuracao_cli.moc"
