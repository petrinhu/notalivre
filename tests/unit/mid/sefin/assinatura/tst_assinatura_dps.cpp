// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do adaptador AssinadorDpsSefin (NL-2.9, DA-2.42). O oráculo das URIs e do namespace é literal, do leiaute
// prodrest_20261020: não lê parametros_infdps.h, para que uma URI trocada lá apareça aqui (mutação).
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/mid/composicao/emissao_nota.h>
#include <nl/mid/sefin/assinatura/assinador_dps_sefin.h>
#include <nl/mid/sefin/leiaute/prodrest_20261020/parametros_infdps.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QtTest>

#include <expected>
#include <string_view>

namespace pk = nl::foundation::pkcs12;
namespace sefin = nl::mid::sefin;

namespace {

// DA-2.38, DA-2.40 e DA-2.61: a tabela por distro diz se o OpenSSL desta distro recusa o SHA-1 da assinatura. Onde recusa, o
// assinador tem de devolver assinatura.algoritmo_bloqueado; onde aceita, a assinatura tem de sair. Nenhum slot pula.
// Devolve "sha1", "algoritmo_bloqueado" ou vazio (distro fora da tabela, ou tabela que não abre). Vazio reprova o teste.
QString esperadoDaDistro()
{
    QString id;
    QString versao = QStringLiteral("rolling");
    QFile os(QStringLiteral("/etc/os-release"));
    if (os.open(QIODevice::ReadOnly)) {
        for (const QString& linha : QString::fromUtf8(os.readAll()).split(QLatin1Char('\n'))) {
            if (linha.startsWith(QStringLiteral("ID="))) {
                id = linha.mid(3).remove(QLatin1Char('"'));
            } else if (linha.startsWith(QStringLiteral("VERSION_ID="))) {
                versao = linha.mid(11).remove(QLatin1Char('"'));
            }
        }
    }
    QFile tabela(qEnvironmentVariable("NL_TABELA_SHA1_ASSINATURA"));
    if (!tabela.open(QIODevice::ReadOnly)) {
        return {};
    }
    for (const QString& linha : QString::fromUtf8(tabela.readAll()).split(QLatin1Char('\n'))) {
        const QStringList campos = linha.simplified().split(QLatin1Char(' '));
        if (linha.startsWith(QLatin1Char('#')) || campos.size() != 3) {
            continue;
        }
        if (campos.at(0) == id && campos.at(1) == versao) {
            return campos.at(2);
        }
    }
    return {};
}

// Distro que recusa o SHA-1 (DA-2.38): o erro tem de ser o código da tabela, e nenhuma assinatura pode sair.
template <typename Resultado>
void exigeRecusaSha1(const Resultado& assinado)
{
    QVERIFY2(!assinado.has_value(), "nesta distro o SHA-1 é recusado (DA-2.38): nenhuma assinatura pode sair");
    QCOMPARE(QString::fromStdString(assinado.error().codigo()), QStringLiteral("assinatura.algoritmo_bloqueado"));
}

const char kIdDps[] = "DPS430510821122233300018100001000000000000001";
const char kDsig[] = "http://www.w3.org/2000/09/xmldsig#";
const char kC14n[] = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
const char kEnveloped[] = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
const char kSha256[] = "http://www.w3.org/2001/04/xmlenc#sha256";
const char kRsaSha256[] = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";
const char kSha1[] = "http://www.w3.org/2000/09/xmldsig#sha1";
const char kRsaSha1[] = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
const char kNamespaceNfse[] = "http://www.sped.fazenda.gov.br/nfse";

// Campo da fixture de teste (NL-2.1), por NL_CERTIFICADOS_DIR. Vazio se o arquivo falta: o teste reprova pela asserção.
QByteArray lerFixture(const char* nome) {
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + QString::fromUtf8(nome));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

QByteArray lerFixtureDps() {
    QFile arquivo(qEnvironmentVariable("NL_FIXTURES_DPS") + QStringLiteral("/valida_dps_mei_minima.xml"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Credencial da fixture cnpj_ok (NL-2.2), com a mesma cadeia que o app usa.
std::expected<pk::CredencialX509, pk::ErroCredencial> credencialDeTeste() {
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    QByteArray senha;
    if (senhaArquivo.open(QIODevice::ReadOnly)) {
        senha = senhaArquivo.readLine().trimmed();
    }
    auto ancoras = pk::Ancoras::dePem(lerFixture("ac.pem"));
    if (!ancoras) {
        return std::unexpected(pk::ErroCredencial::CadeiaInvalida);
    }
    pk::SenhaSegura senhaSegura = pk::SenhaSegura::de(senha);
    pk::CarregadorPkcs12 carregador;
    return carregador.carregar(lerFixture("cnpj_ok.pfx"), senhaSegura, *ancoras);
}

// Conta os nós que casam o XPath no XML (libxml2, local-name e namespace-uri explícitos nas expressões).
int contarNos(const QByteArray& xml, const char* xpath) {
    xmlDocPtr doc = xmlReadMemory(xml.constData(), xml.size(), "assinado.xml", nullptr, 0);
    if (!doc) {
        return -1;
    }
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpath), ctx);
    const int n = obj && obj->nodesetval ? obj->nodesetval->nodeNr : -1;
    xmlXPathFreeObject(obj);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    return n;
}

} // namespace

class TesteAssinaturaDps : public QObject
{
    Q_OBJECT

private slots:
    // A Signature sai com as URIs e o namespace do leiaute, e a referência aponta para o infDPS do leiaute.
    void parametrosDaAssinaturaVemDaPastaDoLeiaute();
    // NL-2.9 (passo a): a raiz de composição liga EmitirNota ao adaptador real; a DPS sai com a Signature do leiaute.
    // DA-2.49: o digest de comparação do E0014 é SHA-256, separado da assinatura (SHA-1, DA-2.2).
    void digestDeComparacaoEhSha256();
    void emissaoLigaAdaptadorRealPelaRaiz();
};

void TesteAssinaturaDps::parametrosDaAssinaturaVemDaPastaDoLeiaute()
{
    const QString esperado = esperadoDaDistro();
    QVERIFY2(esperado == QStringLiteral("sha1") || esperado == QStringLiteral("algoritmo_bloqueado"),
             "a distro desta máquina tem de estar em tabela_sha1_por_distro.txt, com sha1 ou algoritmo_bloqueado (DA-2.40)");
    const QByteArray dps = lerFixtureDps();
    QVERIFY2(!dps.isEmpty(), "a fixture da DPS tem de existir (NL_FIXTURES_DPS)");
    auto credencial = credencialDeTeste();
    QVERIFY2(credencial.has_value(), "a credencial de teste tem de carregar (NL_CERTIFICADOS_DIR)");

    sefin::assinatura::AssinadorDpsSefin assinador(*credencial);
    auto assinado = assinador.assinar(dps, QStringLiteral("DPS430510821122233300018100001000000000000001"));
    if (esperado == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1(assinado);
        return;
    }
    QVERIFY2(assinado.has_value(), "nesta distro o SHA-1 é aceito (DA-2.38): a DPS tem de sair assinada pelo adaptador");
    const QByteArray xml = *assinado;

    // Oráculo literal do leiaute, sem passar por parametros_infdps.h.
    QCOMPARE(contarNos(xml, "//*[local-name()='Reference'][@URI='#DPS430510821122233300018100001000000000000001']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='CanonicalizationMethod'][@Algorithm='http://www.w3.org/TR/2001/REC-xml-c14n-20010315']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='Transform'][@Algorithm='http://www.w3.org/2000/09/xmldsig#enveloped-signature']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='DigestMethod'][@Algorithm='http://www.w3.org/2000/09/xmldsig#sha1']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='SignatureMethod'][@Algorithm='http://www.w3.org/2000/09/xmldsig#rsa-sha1']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='Signature'][namespace-uri()='http://www.w3.org/2000/09/xmldsig#']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='infDPS'][namespace-uri()='http://www.sped.fazenda.gov.br/nfse'][@Id='DPS430510821122233300018100001000000000000001']"), 1);

    // Mesmos valores que a pasta do leiaute declara (o include público), comparados com as constantes literais acima.
    const auto parametros = sefin::prodrest_20261020::parametrosDaAssinaturaDps();
    QVERIFY(parametros.uriDigest == std::string_view(kSha1));
    QVERIFY(parametros.uriAssinatura == std::string_view(kRsaSha1));
    QVERIFY(parametros.uriC14n == std::string_view(kC14n));
    QVERIFY(parametros.uriEnveloped == std::string_view(kEnveloped));
    QVERIFY(parametros.namespaceLeiaute == std::string_view(kNamespaceNfse));
}

void TesteAssinaturaDps::digestDeComparacaoEhSha256()
{
    const auto comparacao = sefin::prodrest_20261020::parametrosDoDigestDeComparacao();
    QVERIFY(comparacao.uriDigest == std::string_view(kSha256));
}

void TesteAssinaturaDps::emissaoLigaAdaptadorRealPelaRaiz()
{
    const QString esperado = esperadoDaDistro();
    QVERIFY2(esperado == QStringLiteral("sha1") || esperado == QStringLiteral("algoritmo_bloqueado"),
             "a distro desta máquina tem de estar em tabela_sha1_por_distro.txt, com sha1 ou algoritmo_bloqueado (DA-2.40)");
    const QByteArray dps = lerFixtureDps();
    QVERIFY2(!dps.isEmpty(), "a fixture da DPS tem de existir (NL_FIXTURES_DPS)");
    auto credencial = credencialDeTeste();
    QVERIFY2(credencial.has_value(), "a credencial de teste tem de carregar (NL_CERTIFICADOS_DIR)");

    nl::mid::composicao::EmissaoNota raiz(*credencial);
    auto assinado = raiz.emitirNota().executar(dps, QStringLiteral("DPS430510821122233300018100001000000000000001"));
    if (esperado == QStringLiteral("algoritmo_bloqueado")) {
        exigeRecusaSha1(assinado);
        return;
    }
    QVERIFY2(assinado.has_value(), "a raiz de composição tem de ligar EmitirNota ao adaptador real de assinatura");
    const QByteArray xml = *assinado;
    QCOMPARE(contarNos(xml, "//*[local-name()='SignatureMethod'][@Algorithm='http://www.w3.org/2000/09/xmldsig#rsa-sha1']"), 1);
    QCOMPARE(contarNos(xml, "//*[local-name()='Reference'][@URI='#DPS430510821122233300018100001000000000000001']"), 1);
}

QTEST_APPLESS_MAIN(TesteAssinaturaDps)
#include "tst_assinatura_dps.moc"
