// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do assinador XMLDSIG (NL-2.3). Cada slot roda por nome no CTest (xmldsig.<slot>), sem vírgula (L-45).
// Oráculos independentes do assinador: xmlsec1 (verificação), xmllint (XSD e C14N), libxml2 (XPath) e o openssl CLI
// (verificação RSA sobre o SignedInfo canonicalizado). Certificados: fixtures sintéticas da NL-2.1, carregadas pelo
// carregador da NL-2.2. Nenhum .pfx real. Falha é asserção (QVERIFY), nunca exceção (L-35).

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/xmldsig/assinador_xml.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include <expected>
#include <string_view>

namespace xd = nl::foundation::xmldsig;
namespace pk = nl::foundation::pkcs12;

namespace {

// Senha da fixture (NL-2.1), sem o fim de linha. Vazia quando o arquivo falta: o teste falha pela asserção.
pk::SenhaSegura senhaDeTeste() {
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (!senhaArquivo.open(QIODevice::ReadOnly)) {
        return pk::SenhaSegura::de(QByteArray());
    }
    return pk::SenhaSegura::de(senhaArquivo.readLine().trimmed());
}

// Campo de /etc/os-release (sem aspas). Sem VERSION_ID (ex.: manjaro, rolling) devolve "rolling".
QString campoDoOsRelease(const QString& campo) {
    QFile arquivo(QStringLiteral("/etc/os-release"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    for (const QString& linha : QString::fromUtf8(arquivo.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        if (linha.startsWith(campo + QLatin1Char('='))) {
            return linha.mid(campo.size() + 1).remove(QLatin1Char('"'));
        }
    }
    return campo == QStringLiteral("VERSION_ID") ? QStringLiteral("rolling") : QString();
}

const QByteArray kIdDps = "DPS430510821122233300018100001000000000000001";
const char kC14n[] = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
const char kEnveloped[] = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
const char kSha1[] = "http://www.w3.org/2000/09/xmldsig#sha1";
const char kRsaSha1[] = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
const char kSha256[] = "http://www.w3.org/2001/04/xmlenc#sha256";
const char kRsaSha256[] = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";

// Parâmetros de teste: os URIs chegam por aqui, como viriam da pasta do leiaute (DA-2.2).
xd::ParametrosAssinatura parametrosSha1() {
    return {kC14n, kEnveloped, kSha1, kRsaSha1};
}

xd::ParametrosAssinatura parametrosSha256() {
    return {kC14n, kEnveloped, kSha256, kRsaSha256};
}

QString caminhoFixture(const char* nome) {
    return qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + QString::fromUtf8(nome);
}

QByteArray lerFixture(const char* nome) {
    QFile arquivo(caminhoFixture(nome));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

QByteArray lerArquivo(const QString& caminho) {
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

QByteArray dpsDeReferencia() {
    return lerArquivo(qEnvironmentVariable("NL_FIXTURES_DPS") + QStringLiteral("/valida_dps_mei_minima.xml"));
}

// Credencial da fixture cnpj_ok (NL-2.2): mesma cadeia de confiança que o app usa.
std::expected<pk::CredencialX509, pk::ErroCredencial> credencialDeTeste(pk::SenhaSegura& senha) {
    auto ancoras = pk::Ancoras::dePem(lerFixture("ac.pem"));
    if (!ancoras) {
        return std::unexpected(pk::ErroCredencial::CadeiaInvalida);
    }
    pk::CarregadorPkcs12 carregador;
    return carregador.carregar(lerFixture("cnpj_ok.pfx"), senha, *ancoras);
}

std::expected<QByteArray, xd::ErroAssinatura> assinarComCredencial(const QByteArray& xml,
                                                                   const xd::ParametrosAssinatura& parametros) {
    pk::SenhaSegura senha = pk::SenhaSegura::de(QByteArray());
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    if (senhaArquivo.open(QIODevice::ReadOnly)) {
        senha = pk::SenhaSegura::de(senhaArquivo.readLine().trimmed());
    }
    auto credencial = credencialDeTeste(senha);
    if (!credencial) {
        return std::unexpected(xd::ErroAssinatura::MaterialInvalido);
    }
    xd::MaterialAssinatura material{credencial->certificado(), credencial->chavePrivada()};
    return xd::AssinadorXml::assinarEnveloped(xml, QStringLiteral("DPS430510821122233300018100001000000000000001"),
                                              parametros, material);
}

// Versão do xmlsec1 de linha de comando: a busca estrita da 1.3 precisa de --lax-key-search (DA-2.28).
bool xmlsec1ePelo13(const QString& xmlsec1) {
    QProcess p;
    p.start(xmlsec1, {QStringLiteral("--version")});
    p.waitForFinished(30000);
    const QRegularExpression re(QStringLiteral("xmlsec1 (\\d+)\\.(\\d+)"));
    const auto m = re.match(QString::fromUtf8(p.readAllStandardOutput() + p.readAllStandardError()));
    return m.hasMatch() && (m.captured(1).toInt() > 1 || (m.captured(1).toInt() == 1 && m.captured(2).toInt() >= 3));
}

bool executar(const QString& programa, const QStringList& argumentos, const QByteArray& entrada = {}) {
    QProcess p;
    p.start(programa, argumentos);
    p.write(entrada);
    p.closeWriteChannel();
    if (!p.waitForFinished(120000)) {
        return false;
    }
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

// xmlsec1 --verify com a AC de teste em --trusted-pem. Devolve verdadeiro só com rc 0.
bool verificarComXmlsec1(const QByteArray& xml) {
    const QString xmlsec1 = qEnvironmentVariable("NL_XMLSEC1");
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return false;
    }
    const QString arquivo = dir.filePath(QStringLiteral("assinado.xml"));
    QFile f(arquivo);
    if (!f.open(QIODevice::WriteOnly) || f.write(xml) != xml.size()) {
        return false;
    }
    f.close();
    QStringList args{QStringLiteral("--verify")};
    if (xmlsec1ePelo13(xmlsec1)) {
        args << QStringLiteral("--lax-key-search");
    }
    args << QStringLiteral("--id-attr:Id") << QStringLiteral("infDPS") << QStringLiteral("--trusted-pem")
         << caminhoFixture("ac.pem") << arquivo;
    return executar(xmlsec1, args);
}

// Conta os nós que casam o XPath no XML (libxml2, sem namespace: local-name()).
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

QString textoDoXPath(const QByteArray& xml, const char* xpath) {
    xmlDocPtr doc = xmlReadMemory(xml.constData(), xml.size(), "assinado.xml", nullptr, 0);
    if (!doc) {
        return {};
    }
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    xmlXPathObjectPtr obj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(xpath), ctx);
    QString out;
    if (obj && obj->nodesetval && obj->nodesetval->nodeNr > 0) {
        const xmlChar* t = xmlNodeGetContent(obj->nodesetval->nodeTab[0]);
        if (t) {
            out = QString::fromUtf8(reinterpret_cast<const char*>(t));
            xmlFree(const_cast<xmlChar*>(t));
        }
    }
    xmlXPathFreeObject(obj);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    return out;
}

} // namespace

class TesteAssinadorXml : public QObject {
    Q_OBJECT

private slots:
    void assinaturaConfereNoXmlsec1Cli() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QVERIFY2(verificarComXmlsec1(*assinado), "o xmlsec1 recusou a assinatura do assinador");
    }

    void xmlAlteradoDepoisDeAssinarReprova() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QByteArray adulterado = *assinado;
        adulterado.replace("<vServ>100.00", "<vServ>999.00");
        QVERIFY2(adulterado != *assinado, "a adulteração não alterou o XML");
        QVERIFY2(!verificarComXmlsec1(adulterado), "XML alterado depois de assinar tinha de reprovar");
    }

    void xmlAssinadoContinuaValidoNoXsd() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        const QString xsd = qEnvironmentVariable("NL_XSD_PRODREST_DIR") + QStringLiteral("/DPS_v1.01.xsd");
        QVERIFY2(executar(qEnvironmentVariable("NL_XMLLINT"),
                          {QStringLiteral("--noout"), QStringLiteral("--nonet"), QStringLiteral("--schema"), xsd,
                           QStringLiteral("-")},
                          *assinado),
                 "o XML assinado tem de continuar válido no XSD");
    }

    void signatureIrmaDeInfDps() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(contarNos(*assinado, "/*[local-name()='DPS']/*[local-name()='Signature']"), 1);
        QCOMPARE(contarNos(*assinado, "/*[local-name()='DPS']/*[local-name()='infDPS']"), 1);
        QCOMPARE(contarNos(*assinado, "//*[local-name()='infDPS']/*[local-name()='Signature']"), 0);
    }

    void referenciaApontaParaIdDoInfDps() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(textoDoXPath(*assinado, "string(//*[local-name()='Reference']/@URI)"),
                 QStringLiteral("#") + QString::fromUtf8(kIdDps));
    }

    void keyInfoSoComX509Certificate() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(contarNos(*assinado, "//*[local-name()='KeyInfo']/*"), 1);
        QCOMPARE(contarNos(*assinado, "//*[local-name()='KeyInfo']/*[local-name()='X509Data']/*"), 1);
        QCOMPARE(contarNos(*assinado, "//*[local-name()='KeyInfo']/*[local-name()='X509Data']"
                                      "/*[local-name()='X509Certificate']"), 1);
    }

    void namespaceHerdadoEntraNaCanonicalizacao() {
        // O infDPS da referência não redeclara o xmlns: a canonicalização do subconjunto tem de herdar o da raiz.
        const QByteArray base = dpsDeReferencia();
        QVERIFY2(!base.contains("<infDPS xmlns"), "a fixture não pode redeclarar o namespace no infDPS");
        auto assinado = assinarComCredencial(base, parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QVERIFY2(verificarComXmlsec1(*assinado), "sem o namespace herdado o digest não confere no xmlsec1");
    }

    void algoritmoVemDaPastaDoLeiaute() {
        // Trocar o algoritmo só pelos parâmetros: SHA-256 entra no XML e o SHA-1 sai, e o xmlsec1 continua verde.
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha256());
        QVERIFY2(assinado.has_value(), "o assinador não aceitou os parâmetros de SHA-256");
        QVERIFY2(assinado->contains("xmldsig-more#rsa-sha256"), "o SignatureMethod tem de vir dos parâmetros");
        QVERIFY2(!assinado->contains("xmldsig#rsa-sha1"), "nenhum rsa-sha1 pode sobrar com os parâmetros de SHA-256");
        QVERIFY2(verificarComXmlsec1(*assinado), "a assinatura SHA-256 tem de verificar no xmlsec1");
    }

    void digestValueIgualAoDouradoEmTodasAsImagens() {
        // Dourado gravado a partir da primeira medição, comparado nas quatro imagens e no SDK (DA-2.10).
        const QString dourado = QString::fromUtf8(
            lerArquivo(qEnvironmentVariable("NL_FIXTURES_XMLDSIG") + QStringLiteral("/digest_infdps.txt"))).trimmed();
        QVERIFY2(!dourado.isEmpty(), "falta o dourado tests/fixtures/xmldsig/digest_infdps.txt");
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(textoDoXPath(*assinado, "string(//*[local-name()='DigestValue'])"), dourado);
    }

    void assinaturaVerificaComAChavePublicaDaFixture() {
        // Segundo oráculo: openssl dgst -verify com a chave pública da fixture, sobre o SignedInfo canonicalizado
        // pelo xmllint. O xmlsec1 não entra nesta conta.
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        const int ini = assinado->indexOf("<SignedInfo");
        const int fim = assinado->indexOf("</SignedInfo>");
        QVERIFY2(ini >= 0 && fim > ini, "sem SignedInfo no XML assinado");
        const QByteArray trecho = assinado->mid(ini, fim - ini + int(sizeof("</SignedInfo>") - 1));
        const QString valorAssinatura = textoDoXPath(*assinado, "string(//*[local-name()='SignatureValue'])");
        QVERIFY2(!valorAssinatura.isEmpty(), "sem SignatureValue");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString docSignedInfo = dir.filePath(QStringLiteral("signedinfo.xml"));
        QFile f(docSignedInfo);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + trecho);
        f.close();
        const QByteArray canonico = [&] {
            QProcess p;
            p.start(qEnvironmentVariable("NL_XMLLINT"), {QStringLiteral("--c14n"), docSignedInfo});
            p.waitForFinished(60000);
            return p.readAllStandardOutput();
        }();
        QVERIFY2(!canonico.isEmpty(), "o xmllint não canonicalizou o SignedInfo");
        QFile chaveArquivo(dir.filePath(QStringLiteral("assinado.bin")));
        QVERIFY(chaveArquivo.open(QIODevice::WriteOnly));
        chaveArquivo.write(canonico);
        chaveArquivo.close();
        const QString certPem = caminhoFixture("cnpj_ok.pem");
        QVERIFY2(QFile::exists(certPem), "falta o certificado PEM da fixture cnpj_ok");
        QProcess chave;
        chave.start(QStringLiteral("openssl"), {QStringLiteral("x509"), QStringLiteral("-in"), certPem,
                                                QStringLiteral("-pubkey"), QStringLiteral("-noout")});
        QVERIFY(chave.waitForFinished(60000));
        const QByteArray chavePublica = chave.readAllStandardOutput();
        QVERIFY2(!chavePublica.isEmpty(), "openssl x509 -pubkey não extraiu a chave pública da fixture");
        QFile chaveArquivo(dir.filePath(QStringLiteral("chave_publica.pem")));
        QVERIFY(chaveArquivo.open(QIODevice::WriteOnly));
        chaveArquivo.write(chavePublica);
        chaveArquivo.close();
        const QByteArray sig = QByteArray::fromBase64(valorAssinatura.toUtf8());
        QFile sigArquivo(dir.filePath(QStringLiteral("assinatura.bin")));
        QVERIFY(sigArquivo.open(QIODevice::WriteOnly));
        sigArquivo.write(sig);
        sigArquivo.close();
        QVERIFY2(executar(QStringLiteral("openssl"),
                          {QStringLiteral("dgst"), QStringLiteral("-sha1"), QStringLiteral("-verify"),
                           dir.filePath(QStringLiteral("chave_publica.pem")), QStringLiteral("-signature"),
                           dir.filePath(QStringLiteral("assinatura.bin")), dir.filePath(QStringLiteral("assinado.bin"))}),
                 "openssl dgst -verify recusou o SignedInfo com a chave pública da fixture");
    }

    void algoritmoBloqueadoViraErroProprio() {
        // Digest que o provider default do OSSL_LIB_CTX não entrega: vira erro próprio, nunca falha silenciosa (DA-2.10).
        const xd::ParametrosAssinatura bloqueado{kC14n, kEnveloped,
                                                 "http://www.w3.org/2001/04/xmldsig-more#md4", kRsaSha1};
        auto assinado = assinarComCredencial(dpsDeReferencia(), bloqueado);
        QVERIFY2(!assinado.has_value(), "digest bloqueado não pode assinar");
        QVERIFY2(assinado.error() == xd::ErroAssinatura::AlgoritmoBloqueado, "o erro tinha de ser algoritmo bloqueado");
        QVERIFY2(xd::codigo(assinado.error()) == std::string_view("assinatura.algoritmo_bloqueado"),
                 "o código do erro tem de ser assinatura.algoritmo_bloqueado");
    }

    void materialNuloDevolveErroProprio() {
        // Ponteiro nulo é erro próprio, nunca falha silenciosa nem desreferência (DA-2.36).
        pk::SenhaSegura senha = senhaDeTeste();
        auto credencial = credencialDeTeste(senha);
        QVERIFY2(credencial.has_value(), "a fixture cnpj_ok tem de carregar");
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        const xd::MaterialAssinatura semCertificado{nullptr, credencial->chavePrivada()};
        auto a = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), id, parametrosSha1(), semCertificado);
        QVERIFY2(!a.has_value(), "certificado nulo não pode assinar");
        QVERIFY2(a.error() == xd::ErroAssinatura::MaterialInvalido, "certificado nulo tem de ser MaterialInvalido");
        const xd::MaterialAssinatura semChave{credencial->certificado(), nullptr};
        auto b = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), id, parametrosSha1(), semChave);
        QVERIFY2(!b.has_value(), "chave nula não pode assinar");
        QVERIFY2(b.error() == xd::ErroAssinatura::MaterialInvalido, "chave nula tem de ser MaterialInvalido");
    }

    void materialEmprestadoNaoLiberaNemCopia() {
        // Roda sob ASan (preset asan-ubsan): o assinador não libera, não copia e não mexe na contagem de referência.
        // Sem uso depois de liberar e sem vazamento, o sanitizer acusa. Aqui: ponteiros iguais, credencial viva, chave usável.
        pk::SenhaSegura senha = senhaDeTeste();
        auto credencial = credencialDeTeste(senha);
        QVERIFY2(credencial.has_value(), "a fixture cnpj_ok tem de carregar");
        x509_st* const cert = credencial->certificado();
        evp_pkey_st* const chave = credencial->chavePrivada();
        const xd::MaterialAssinatura material{cert, chave};
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        auto assinado = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), id, parametrosSha1(), material);
        QVERIFY2(assinado.has_value(), "o assinador tem de assinar com o material emprestado");
        QVERIFY2(credencial->certificado() == cert && credencial->chavePrivada() == chave,
                 "o assinador não pode trocar os ponteiros da credencial");
        QVERIFY2(!credencial->descartada(), "o assinador não pode descartar a credencial");
        QVERIFY2(EVP_PKEY_size(chave) > 0, "a chave tem de continuar usável depois da chamada");
    }

    void algoritmoDaTabelaPorDistro() {
        // DA-2.38: a expectativa sai da tabela por distro, lida do /etc/os-release. sha1 = assina sem a variável;
        // algoritmo_bloqueado = o OSSL_LIB_CTX próprio recusa (o esperado no fedora44 se a medição disser isso).
        const QString id = campoDoOsRelease(QStringLiteral("ID"));
        const QString versao = campoDoOsRelease(QStringLiteral("VERSION_ID"));
        QVERIFY2(!id.isEmpty(), "/etc/os-release sem ID");
        const QString tabela = qEnvironmentVariable("NL_FIXTURES_XMLDSIG") + QStringLiteral("/tabela_sha1_por_distro.txt");
        QString esperado;
        for (const QString& linha : QString::fromUtf8(lerArquivo(tabela)).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            if (linha.startsWith(QLatin1Char('#'))) {
                continue;
            }
            const QStringList colunas = linha.simplified().split(QLatin1Char(' '));
            if (colunas.size() == 3 && colunas[0] == id && colunas[1] == versao) {
                esperado = colunas[2];
            }
        }
        QVERIFY2(!esperado.isEmpty(), "distro sem linha em tests/fixtures/xmldsig/tabela_sha1_por_distro.txt");
        QVERIFY2(esperado == QStringLiteral("sha1") || esperado == QStringLiteral("algoritmo_bloqueado"),
                 "expectativa da tabela ainda não medida (PENDENTE): medir e gravar antes do verde");
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        if (esperado == QStringLiteral("sha1")) {
            QVERIFY2(assinado.has_value(), "esta distro tem de assinar SHA-1 sem a variável");
        } else {
            QVERIFY2(!assinado.has_value(), "esta distro recusa SHA-1: não pode assinar");
            QVERIFY2(assinado.error() == xd::ErroAssinatura::AlgoritmoBloqueado, "o erro tem de ser algoritmo bloqueado");
        }
    }

    void assinaSha1NoSdkKdeSemVariavel() {
        // Roda só no build Flatpak (SDK KDE 6.11, runtime medido na NL-2.3, passo 0). A variável tem de estar ausente.
        QVERIFY2(!qEnvironmentVariableIsSet("OPENSSL_ENABLE_SHA1_SIGNATURES"),
                 "a variável de SHA-1 não pode existir no processo do app");
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o SDK KDE 6.11 sem a variável tem de assinar SHA-1");
    }
};

QTEST_APPLESS_MAIN(TesteAssinadorXml)
#include "tst_assinador_xml.moc"
