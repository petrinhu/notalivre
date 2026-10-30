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

#include <optional>

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/provider.h>
#include <openssl/x509.h>

#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstdlib>
#include <expected>
#include <memory>
#include <string_view>
#include <thread>

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
const char kNsLeiaute[] = "http://www.sped.fazenda.gov.br/nfse";

// NFS-e no formato do XSD (rodada 5, NFSe/infNFSe/DPS/infDPS): a DPS de referência dentro de infNFSe, dentro da raiz NFSe.
QByteArray nfseDoXsd(const QByteArray& dps) {
    const QByteArray corpo = dps.mid(dps.indexOf("<DPS"));
    return QByteArrayLiteral("<NFSe xmlns=\"http://www.sped.fazenda.gov.br/nfse\"><infNFSe Id=\"NFS00000000000000000000000000000000000000000000000000000\">") +
           corpo + QByteArrayLiteral("</infNFSe></NFSe>");
}

xd::ParametrosAssinatura parametrosNfseSha256() {
    return {kC14n, kEnveloped, kSha256, kRsaSha256, "infDPS", "DPS", kNsLeiaute, "NFSe", "infNFSe"};
}

xd::ParametrosAssinatura parametrosSha1() {
    return {kC14n, kEnveloped, kSha1, kRsaSha1, "infDPS", "DPS", kNsLeiaute};
}

xd::ParametrosAssinatura parametrosSha256() {
    return {kC14n, kEnveloped, kSha256, kRsaSha256, "infDPS", "DPS", kNsLeiaute};
}

// Ouvinte TCP em 127.0.0.1 (NL-2.9, condição do CISO sobre DTD remoto): conta cada conexão aceita e a fecha. Thread
// própria, porque o parse é síncrono e o servidor precisa existir fora da thread do teste. Porta efêmera.
class OuvinteTcp {
public:
    OuvinteTcp() {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in endereco{};
        endereco.sin_family = AF_INET;
        endereco.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t tamanho = sizeof(endereco);
        if (fd_ < 0 || ::bind(fd_, reinterpret_cast<sockaddr*>(&endereco), sizeof(endereco)) != 0 ||
            ::listen(fd_, 16) != 0 || ::getsockname(fd_, reinterpret_cast<sockaddr*>(&endereco), &tamanho) != 0) {
            return;
        }
        porta_ = ntohs(endereco.sin_port);
        ativo_ = true;
        thread_ = std::thread([this] { laco(); });
    }
    ~OuvinteTcp() { parar(); }
    OuvinteTcp(const OuvinteTcp&) = delete;
    OuvinteTcp& operator=(const OuvinteTcp&) = delete;

    int porta() const { return porta_; }
    int conexoes() const { return conexoes_.load(); }

    // Para o laço e espera a thread. Depois de parar, conexoes() é o total final.
    void parar() {
        ativo_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    void laco() {
        while (ativo_) {
            pollfd espera{fd_, POLLIN, 0};
            if (::poll(&espera, 1, 20) > 0) {
                const int conexao = ::accept(fd_, nullptr, nullptr);
                if (conexao >= 0) {
                    conexoes_++;
                    ::close(conexao);
                }
            }
        }
    }

    int fd_ = -1;
    int porta_ = 0;
    std::atomic<bool> ativo_{false};
    std::atomic<int> conexoes_{0};
    std::thread thread_;
};

// Código estável de um erro do assinador, como QString, para comparar nas asserções.
QString codigoDe(xd::ErroAssinatura erro) {
    const auto codigo = xd::codigo(erro);
    return QString::fromUtf8(codigo.data(), static_cast<int>(codigo.size()));
}

QByteArray dpsDeReferencia();

// DPS de referência com o xmlns:xsi declarado na raiz (DA-2.40, item 4: o SignedInfo herda esse namespace).
QByteArray dpsComXsiNaRaiz() {
    QByteArray xml = dpsDeReferencia();
    xml.replace("<DPS xmlns=", "<DPS xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=");
    return xml;
}

QByteArray lerArquivo(const QString& caminho);

// Expectativa de SHA-1 de uma distro, lida da tabela versionada (NL-2.9, condição (a) da re-revisão da NL-2.3). Sem linha
// para a distro, devolve nullopt, e a suíte reprova (distroAtualTemLinhaNaTabela).
std::optional<bool> recusaSha1SegundoTabela(const QString& id, const QString& versao) {
    const QByteArray tabela =
        lerArquivo(qEnvironmentVariable("NL_FIXTURES_XMLDSIG") + QStringLiteral("/tabela_sha1_por_distro.txt"));
    std::optional<QString> esperado;
    for (const QString& linha : QString::fromUtf8(tabela).split(QLatin1Char('\n'))) {
        if (linha.startsWith(QLatin1Char('#')) || linha.trimmed().isEmpty()) {
            continue;
        }
        const QStringList colunas = linha.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (colunas.size() == 3 && colunas[0] == id && colunas[1] == versao) {
            esperado = colunas[2];
        }
    }
    if (!esperado) {
        return std::nullopt;
    }
    return *esperado == QStringLiteral("algoritmo_bloqueado");
}

// DA-2.38: a distro que recusa rsa-sha1 (linha algoritmo_bloqueado da tabela) usa rsa-sha256 na suíte. A tabela é a fonte;
// sem linha, o valor é falso aqui e a suíte reprova no teste distroAtualTemLinhaNaTabela.
bool distroRecusaSha1() {
    return recusaSha1SegundoTabela(campoDoOsRelease(QStringLiteral("ID")), campoDoOsRelease(QStringLiteral("VERSION_ID")))
        .value_or(false);
}

xd::ParametrosAssinatura parametrosDaDistro() {
    return distroRecusaSha1() ? parametrosSha256() : parametrosSha1();
}

const char* nomeDoDigestDaDistro() {
    return distroRecusaSha1() ? "sha256" : "sha1";
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
                                                                   const xd::ParametrosAssinatura& parametros,
                                                                   const QString& id = QStringLiteral("DPS430510821122233300018100001000000000000001")) {
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
    return xd::AssinadorXml::assinarEnveloped(xml, id, parametros, material);
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
    if (obj && obj->type == XPATH_STRING && obj->stringval != nullptr) {
        out = QString::fromUtf8(reinterpret_cast<const char*>(obj->stringval));
    } else if (obj && obj->nodesetval && obj->nodesetval->nodeNr > 0) {
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

// Material de PEM lido por contexto OpenSSL só do teste (DA-2.40 adendo). Não passa pelo PKCS#12, que valida a janela
// do certificado e recusaria a fixture vencida antes do assinador. Ordem dos membros = ordem de destruição inversa.
struct MaterialPem {
    std::unique_ptr<OSSL_LIB_CTX, decltype(&OSSL_LIB_CTX_free)> ctx{nullptr, &OSSL_LIB_CTX_free};
    std::unique_ptr<OSSL_PROVIDER, decltype(&OSSL_PROVIDER_unload)> provedor{nullptr, &OSSL_PROVIDER_unload};
    std::unique_ptr<X509, decltype(&X509_free)> cert{nullptr, &X509_free};
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> chave{nullptr, &EVP_PKEY_free};
};

bool lerMaterialPem(const char* certNome, const char* chaveNome, MaterialPem& material) {
    material.ctx.reset(OSSL_LIB_CTX_new());
    if (!material.ctx) {
        return false;
    }
    material.provedor.reset(OSSL_PROVIDER_load(material.ctx.get(), "default"));
    if (!material.provedor) {
        return false;
    }
    const QByteArray certPem = lerArquivo(caminhoFixture(certNome));
    const QByteArray chavePem = lerArquivo(caminhoFixture(chaveNome));
    if (certPem.isEmpty() || chavePem.isEmpty()) {
        return false;
    }
    std::unique_ptr<BIO, decltype(&BIO_free)> bioCert(BIO_new_mem_buf(certPem.constData(), certPem.size()), &BIO_free);
    std::unique_ptr<BIO, decltype(&BIO_free)> bioChave(BIO_new_mem_buf(chavePem.constData(), chavePem.size()), &BIO_free);
    material.cert.reset(PEM_read_bio_X509_AUX(bioCert.get(), nullptr, nullptr, nullptr));
    material.chave.reset(PEM_read_bio_PrivateKey_ex(bioChave.get(), nullptr, nullptr, nullptr, material.ctx.get(), nullptr));
    return material.cert != nullptr && material.chave != nullptr;
}

class TesteAssinadorXml : public QObject {
    Q_OBJECT

private slots:
    void assinaturaConfereNoXmlsec1Cli() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QVERIFY2(verificarComXmlsec1(*assinado), "o xmlsec1 recusou a assinatura do assinador");
    }

    void xmlAlteradoDepoisDeAssinarReprova() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QByteArray adulterado = *assinado;
        adulterado.replace("<vServ>100.00", "<vServ>999.00");
        QVERIFY2(adulterado != *assinado, "a adulteração não alterou o XML");
        QVERIFY2(!verificarComXmlsec1(adulterado), "XML alterado depois de assinar tinha de reprovar");
    }

    void xmlAssinadoContinuaValidoNoXsd() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        const QString xsd = qEnvironmentVariable("NL_XSD_PRODREST_DIR") + QStringLiteral("/DPS_v1.01.xsd");
        QVERIFY2(executar(qEnvironmentVariable("NL_XMLLINT"),
                          {QStringLiteral("--noout"), QStringLiteral("--nonet"), QStringLiteral("--schema"), xsd,
                           QStringLiteral("-")},
                          *assinado),
                 "o XML assinado tem de continuar válido no XSD");
    }

    void signatureIrmaDeInfDps() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(contarNos(*assinado, "/*[local-name()='DPS']/*[local-name()='Signature']"), 1);
        QCOMPARE(contarNos(*assinado, "/*[local-name()='DPS']/*[local-name()='infDPS']"), 1);
        QCOMPARE(contarNos(*assinado, "//*[local-name()='infDPS']/*[local-name()='Signature']"), 0);
    }

    void referenciaApontaParaIdDoInfDps() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        QCOMPARE(textoDoXPath(*assinado, "string(//*[local-name()='Reference']/@URI)"),
                 QStringLiteral("#") + QString::fromUtf8(kIdDps));
    }

    void keyInfoSoComX509Certificate() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
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
        auto assinado = assinarComCredencial(base, parametrosDaDistro());
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
        // Dourado gravado a partir da primeira medição (DA-2.10), por algoritmo. Nesta distro, o da distro.
        const QString algoritmo = QString::fromUtf8(nomeDoDigestDaDistro());
        const QString dourado = QString::fromUtf8(lerArquivo(qEnvironmentVariable("NL_FIXTURES_XMLDSIG") +
                                                             QStringLiteral("/digest_infdps_") + algoritmo +
                                                             QStringLiteral(".txt"))).trimmed();
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado");
        const QString valor = textoDoXPath(*assinado, "string(//*[local-name()='DigestValue'])");
        qInfo("DIGEST algoritmo=%s valor=%s", qPrintable(algoritmo), qPrintable(valor));
        QVERIFY2(!dourado.isEmpty(), "falta o dourado tests/fixtures/xmldsig/digest_infdps_<algoritmo>.txt");
        QCOMPARE(valor, dourado);
    }

    void digestSha256IgualAoDouradoEmTodasAsImagens() {
        // O SHA-256 é o mesmo em todas as imagens, inclusive o fedora44 (DA-2.38): o dourado é único.
        const QString dourado = QString::fromUtf8(lerArquivo(qEnvironmentVariable("NL_FIXTURES_XMLDSIG") +
                                                             QStringLiteral("/digest_infdps_sha256.txt"))).trimmed();
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha256());
        QVERIFY2(assinado.has_value(), "o assinador não produziu XML assinado com SHA-256");
        const QString valor = textoDoXPath(*assinado, "string(//*[local-name()='DigestValue'])");
        qInfo("DIGEST algoritmo=sha256 valor=%s", qPrintable(valor));
        QVERIFY2(!dourado.isEmpty(), "falta o dourado tests/fixtures/xmldsig/digest_infdps_sha256.txt");
        QCOMPARE(valor, dourado);
    }

    void assinaturaVerificaComAChavePublicaDaFixture() {
        // Segundo oráculo: openssl dgst -verify com a chave pública da fixture, sobre o SignedInfo canonicalizado
        // pelo xmllint. O xmlsec1 não entra nesta conta.
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
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
        QFile canonicoArquivo(dir.filePath(QStringLiteral("assinado.bin")));
        QVERIFY(canonicoArquivo.open(QIODevice::WriteOnly));
        canonicoArquivo.write(canonico);
        canonicoArquivo.close();
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
                          {QStringLiteral("dgst"), QString::fromUtf8(nomeDoDigestDaDistro()).prepend(QLatin1Char('-')), QStringLiteral("-verify"),
                           dir.filePath(QStringLiteral("chave_publica.pem")), QStringLiteral("-signature"),
                           dir.filePath(QStringLiteral("assinatura.bin")), dir.filePath(QStringLiteral("assinado.bin"))}),
                 "openssl dgst -verify recusou o SignedInfo com a chave pública da fixture");
    }

    void algoritmoBloqueadoViraErroProprio() {
        // Digest que o provider default do OSSL_LIB_CTX não entrega: vira erro próprio, nunca falha silenciosa (DA-2.10).
        const xd::ParametrosAssinatura bloqueado{kC14n, kEnveloped,
                                                 "http://www.w3.org/2001/04/xmldsig-more#md4", kRsaSha1,
                                                 "infDPS", "DPS", kNsLeiaute};
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
        auto assinado = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), id, parametrosDaDistro(), material);
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

    // DA-2.40 (d). Vermelhos antes do código (L-35): cada um falha por asserção da regra.
    void signedInfoCanonicalizadoNoContextoDoDocumento() {
        auto assinado = assinarComCredencial(dpsComXsiNaRaiz(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "a DPS com xmlns:xsi na raiz tem de assinar");
        QVERIFY2(verificarComXmlsec1(*assinado), "o xmlsec1 tem de verificar: o SignedInfo herda o xmlns:xsi da raiz");
    }

    void namespacePrefixadoDaRaizEntraNoDigest() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("<DPS xmlns=", "<DPS xmlns:nfse=\"http://www.sped.fazenda.gov.br/nfse\" xmlns=");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "a DPS com prefixo na raiz tem de assinar");
        QVERIFY2(verificarComXmlsec1(*assinado), "o digest tem de incluir o namespace prefixado herdado da raiz");
    }

    void transformadaEnvelopedExcluiASignatureDoDigest() {
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "a DPS de referência tem de assinar");
        QCOMPARE(assinado->count("<Signature "), qsizetype{1});  // uma só Signature no documento (o espaço exclui SignatureValue)
        QVERIFY2(verificarComXmlsec1(*assinado), "a transformada enveloped tem de deixar o digest igual ao do verificador");
    }

    void xmlJaAssinadoRecusado() {
        auto primeira = assinarComCredencial(dpsDeReferencia(), parametrosDaDistro());
        QVERIFY2(primeira.has_value(), "a DPS de referência tem de assinar");
        auto segunda = assinarComCredencial(*primeira, parametrosDaDistro());
        QVERIFY2(!segunda.has_value(), "XML já assinado nunca recebe outra Signature");
        QCOMPARE(codigoDe(segunda.error()), QStringLiteral("assinatura.ja_assinado"));
        QByteArray dentro = dpsDeReferencia();
        dentro.replace("</infDPS>", "<Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\"/></infDPS>");
        auto dentroDoInfDps = assinarComCredencial(dentro, parametrosDaDistro());
        QVERIFY2(!dentroDoInfDps.has_value(), "Signature dentro do infDPS também conta como já assinado");
        QCOMPARE(codigoDe(dentroDoInfDps.error()), QStringLiteral("assinatura.ja_assinado"));
    }

    void idRepetidoRecusado() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("</DPS>", "<Outro Id=\"DPS430510821122233300018100001000000000000001\"/></DPS>");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(!assinado.has_value(), "dois elementos com o mesmo Id tem de ser recusados");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.referencia_ambigua"));
    }

    void idPorPrefixoNaoCasa() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("<infDPS Id=", "<infDPS xmlns:p=\"urn:teste\" p:Id=");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(!assinado.has_value(), "Id com prefixo não é o Id do infDPS");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.referencia_nao_encontrada"));
    }

    void idNaRaizRecusado() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("<infDPS Id=\"DPS430510821122233300018100001000000000000001\">", "<infDPS>");
        xml.replace("<DPS xmlns=", "<DPS Id=\"DPS430510821122233300018100001000000000000001\" xmlns=");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(!assinado.has_value(), "o Id na raiz DPS não é o elemento de referência");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.referencia_nao_encontrada"));
    }

    void idEmOutroElementoRecusado() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("<infDPS Id=\"DPS430510821122233300018100001000000000000001\">", "<infDPS>");
        xml.replace("</infDPS>", "<Outro Id=\"DPS430510821122233300018100001000000000000001\"/></infDPS>");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(!assinado.has_value(), "o Id num elemento que não é o infDPS não serve de referência");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.referencia_nao_encontrada"));
    }

    // NL-2.9, condição (c): o Id entra no modelo da Signature pela API de árvore da libxml2, nunca por concatenação.
    // Um Id com aspa dupla e um "atributo" dentro do valor não pode abrir atributo novo no Reference.
    void idComAspasNaoInjetaAtributo() {
        const QString idComAspas = QStringLiteral("DPS430510821122233300018100001000000000000001\" Evil=\"x");
        QByteArray xml = dpsDeReferencia();
        xml.replace("<infDPS Id=\"DPS430510821122233300018100001000000000000001\">",
                    "<infDPS Id='DPS430510821122233300018100001000000000000001\" Evil=\"x'>");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro(), idComAspas);
        QVERIFY2(assinado.has_value(), "o Id com aspas é valor XML válido e tem de achar o infDPS");
        QCOMPARE(contarNos(*assinado, "//*[local-name()='Reference'][@Evil]"), 0);
        QCOMPARE(contarNos(*assinado, "//*[local-name()='Reference'][@URI='#DPS430510821122233300018100001000000000000001\" Evil=\"x']"), 1);
    }

    // Rodada 5 (revisão): a NFS-e no formato do XSD dá o mesmo digest que a DPS, e um campo alterado dentro dela difere.
    void digestDaNfseNoFormatoDoXsdIgualAoDaDps() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        auto daDps = xd::digestCanonicoDaReferencia(dpsDeReferencia(), id, parametrosNfseSha256());
        auto daNfse = xd::digestCanonicoDaReferencia(nfseDoXsd(dpsDeReferencia()), id, parametrosNfseSha256());
        QVERIFY2(daDps.has_value() && daNfse.has_value(), "a NFS-e no formato do XSD tem de dar digest");
        QCOMPARE(*daNfse, *daDps);
    }

    void digestDaNfseAlteradaDifere() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray alterada = dpsDeReferencia();
        alterada.replace("</infDPS>", "<xAlterado>1</xAlterado></infDPS>");
        auto daDps = xd::digestCanonicoDaReferencia(dpsDeReferencia(), id, parametrosNfseSha256());
        auto daNfse = xd::digestCanonicoDaReferencia(nfseDoXsd(alterada), id, parametrosNfseSha256());
        QVERIFY2(daDps.has_value() && daNfse.has_value(), "as duas formas tem de dar digest");
        QVERIFY2(*daDps != *daNfse, "um campo alterado dentro da NFS-e muda o digest");
    }

    void digestNfseIdRepetidoDaErro() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray xml = nfseDoXsd(dpsDeReferencia());
        xml.replace("</infNFSe>", "<Outro Id=\"DPS430510821122233300018100001000000000000001\"/></infNFSe>");
        auto digest = xd::digestCanonicoDaReferencia(xml, id, parametrosNfseSha256());
        QVERIFY2(!digest.has_value(), "Id repetido dentro da NFS-e não pode dar digest");
        QCOMPARE(codigoDe(digest.error()), QStringLiteral("assinatura.referencia_ambigua"));
    }

    // Guarda de caminho: infDPS fora de NFSe/infNFSe/DPS é recusado, mesmo que o Id esteja certo.
    void digestInfDpsForaDoCaminhoFixoRecusado() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray xml = nfseDoXsd(dpsDeReferencia());
        xml.replace("<infNFSe Id=", "<outro><infNFSe Id=");
        xml.replace("</infNFSe></NFSe>", "</infNFSe></outro></NFSe>");
        auto digest = xd::digestCanonicoDaReferencia(xml, id, parametrosNfseSha256());
        QVERIFY2(!digest.has_value(), "infDPS fora do caminho fixo do leiaute não pode servir de referência");
        QCOMPARE(codigoDe(digest.error()), QStringLiteral("assinatura.referencia_nao_encontrada"));
    }

    // Digest canônico do infDPS (átomo do NL-2.8, adendo da DA-2.41): o mesmo digest antes e depois de assinar.
    void digestIgualAntesEDepoisDeAssinar() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        auto antes = xd::digestCanonicoDaReferencia(dpsDeReferencia(), id, parametrosSha256());
        QVERIFY2(antes.has_value(), "a DPS de referência sem assinatura tem de dar digest");
        QCOMPARE(antes->size(), qsizetype{32});
        auto assinada = assinarComCredencial(dpsDeReferencia(), parametrosSha256());
        QVERIFY2(assinada.has_value(), "a DPS de referência tem de assinar");
        auto depois = xd::digestCanonicoDaReferencia(*assinada, id, parametrosSha256());
        QVERIFY2(depois.has_value(), "a DPS assinada tem de dar digest, com a Signature fora");
        QCOMPARE(*depois, *antes);
    }

    void digestMudaComCampoAlterado() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray alterada = dpsDeReferencia();
        alterada.replace("</infDPS>", "<xAlterado>1</xAlterado></infDPS>");
        auto original = xd::digestCanonicoDaReferencia(dpsDeReferencia(), id, parametrosSha256());
        auto mudada = xd::digestCanonicoDaReferencia(alterada, id, parametrosSha256());
        QVERIFY2(original.has_value() && mudada.has_value(), "as duas DPS tem de dar digest");
        QVERIFY2(*original != *mudada, "um campo alterado no infDPS muda o digest");
    }

    void digestIdRepetidoDaErro() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray xml = dpsDeReferencia();
        xml.replace("</DPS>", "<Outro Id=\"DPS430510821122233300018100001000000000000001\"/></DPS>");
        auto digest = xd::digestCanonicoDaReferencia(xml, id, parametrosSha256());
        QVERIFY2(!digest.has_value(), "Id repetido não pode dar digest");
        QCOMPARE(codigoDe(digest.error()), QStringLiteral("assinatura.referencia_ambigua"));
    }

    void digestIdAusenteDaErro() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray xml = dpsDeReferencia();
        xml.replace("<infDPS Id=\"DPS430510821122233300018100001000000000000001\">", "<infDPS>");
        auto digest = xd::digestCanonicoDaReferencia(xml, id, parametrosSha256());
        QVERIFY2(!digest.has_value(), "Id ausente não pode dar digest");
        QCOMPARE(codigoDe(digest.error()), QStringLiteral("assinatura.referencia_nao_encontrada"));
    }

    void digestDtdDaErro() {
        const QStringView id = QStringLiteral("DPS430510821122233300018100001000000000000001");
        QByteArray comEntidade = dpsDeReferencia();
        comEntidade.replace("<DPS xmlns=", "<!DOCTYPE DPS [<!ENTITY e \"x\">]>\n<DPS xmlns=");
        auto digest = xd::digestCanonicoDaReferencia(comEntidade, id, parametrosSha256());
        QVERIFY2(!digest.has_value(), "DTD com entidade não pode dar digest");
        QCOMPARE(codigoDe(digest.error()), QStringLiteral("assinatura.xml_com_dtd"));
    }

    void xmlComDtdRecusado() {
        QByteArray comAtlist = dpsDeReferencia();
        comAtlist.replace("<DPS xmlns=", "<!DOCTYPE DPS [<!ATTLIST infDPS Id ID #IMPLIED>]>\n<DPS xmlns=");
        auto semDtd = assinarComCredencial(comAtlist, parametrosDaDistro());
        QVERIFY2(!semDtd.has_value(), "DTD com ATTLIST de tipo ID tem de ser recusado");
        QCOMPARE(codigoDe(semDtd.error()), QStringLiteral("assinatura.xml_com_dtd"));
        QByteArray comEntidade = dpsDeReferencia();
        comEntidade.replace("<DPS xmlns=", "<!DOCTYPE DPS [<!ENTITY e \"x\">]>\n<DPS xmlns=");
        auto comEnt = assinarComCredencial(comEntidade, parametrosDaDistro());
        QVERIFY2(!comEnt.has_value(), "DTD com entidade tem de ser recusado");
        QCOMPARE(codigoDe(comEnt.error()), QStringLiteral("assinatura.xml_com_dtd"));
    }

    // NL-2.9, condição (d): o parser para no primeiro DOCTYPE, antes de ler o subconjunto interno. Um subconjunto
    // malformado não pode virar xml_malformado: a recusa de DTD sai antes de qualquer declaração ser processada.
    void dtdComSubconjuntoMalformadoRecusadoAntesDoParse() {
        QByteArray xml = dpsDeReferencia();
        xml.replace("<DPS xmlns=", "<!DOCTYPE DPS [<!ENTITY e \"x>\n<DPS xmlns=");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(!assinado.has_value(), "DTD com subconjunto malformado tem de ser recusado");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.xml_com_dtd"));
    }

    // NL-2.9, reforço sobre o DTD: um DOCTYPE SYSTEM ou uma entidade externa apontando para a rede não pode abrir conexão.
    // O ouvinte em 127.0.0.1 conta as conexões aceitas; a espera é zero em ambas as variantes.
    void xmlComDoctypeSystemRemotoNaoAcessaARede() {
        OuvinteTcp ouvinte;
        QVERIFY2(ouvinte.porta() != 0, "o ouvinte TCP local tem de subir");
        const QByteArray url = "http://127.0.0.1:" + QByteArray::number(ouvinte.porta()) + "/dtd.dtd";
        QByteArray comSystem = dpsDeReferencia();
        comSystem.replace("<DPS xmlns=", "<!DOCTYPE DPS SYSTEM \"" + url + "\">\n<DPS xmlns=");
        const auto semDtd = assinarComCredencial(comSystem, parametrosDaDistro());
        QByteArray comEntidade = dpsDeReferencia();
        comEntidade.replace("<DPS xmlns=", "<!DOCTYPE DPS [<!ENTITY e SYSTEM \"" + url + "\">]>\n<DPS xmlns=");
        comEntidade.replace("</DPS>", "&e;</DPS>");
        const auto comEnt = assinarComCredencial(comEntidade, parametrosDaDistro());
        ouvinte.parar();
        QVERIFY2(!semDtd.has_value(), "DOCTYPE SYSTEM tem de ser recusado");
        QCOMPARE(codigoDe(semDtd.error()), QStringLiteral("assinatura.xml_com_dtd"));
        QVERIFY2(!comEnt.has_value(), "entidade externa tem de ser recusada");
        QCOMPARE(codigoDe(comEnt.error()), QStringLiteral("assinatura.xml_com_dtd"));
        QCOMPARE(ouvinte.conexoes(), 0);
    }

    void variavelDoFedoraValeNoContextoProprio() {
        // DA-2.40 (c): o patch do fedora lê a variável em tempo de chamada. Sem ela, fedora44 recusa SHA-1; com ela, assina.
        ::unsetenv("OPENSSL_ENABLE_SHA1_SIGNATURES");
        auto sem = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        ::setenv("OPENSSL_ENABLE_SHA1_SIGNATURES", "1", 1);
        auto com = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        ::unsetenv("OPENSSL_ENABLE_SHA1_SIGNATURES");
        QVERIFY2(com.has_value(), "com a variável, o contexto próprio assina SHA-1");
        if (distroRecusaSha1()) {
            QVERIFY2(!sem.has_value(), "no fedora44, sem a variável, SHA-1 é recusado");
            QCOMPARE(codigoDe(sem.error()), QStringLiteral("assinatura.algoritmo_bloqueado"));
        } else {
            QVERIFY2(sem.has_value(), "fora do fedora44, SHA-1 assina sem a variável");
        }
    }

    void materialVencidoNaHoraDeAssinarRecusado() {
        MaterialPem pem;
        QVERIFY2(lerMaterialPem("vencido.pem", "vencido.key.pem", pem), "a folha vencida tem de ler por PEM no contexto do teste");
        xd::MaterialAssinatura material{pem.cert.get(), pem.chave.get()};
        auto assinado = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), QStringLiteral("DPS430510821122233300018100001000000000000001"), parametrosDaDistro(), material);
        QVERIFY2(!assinado.has_value(), "certificado vencido na hora de assinar tem de ser recusado");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.material_invalido"));
    }

    void materialAindaNaoValidoRecusado() {
        MaterialPem pem;
        QVERIFY2(lerMaterialPem("folha_futura.pem", "folha_futura.key.pem", pem), "a folha com notBefore futuro tem de ler por PEM");
        xd::MaterialAssinatura material{pem.cert.get(), pem.chave.get()};
        auto assinado = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(), QStringLiteral("DPS430510821122233300018100001000000000000001"), parametrosDaDistro(), material);
        QVERIFY2(!assinado.has_value(), "certificado ainda não válido na hora de assinar tem de ser recusado");
        QCOMPARE(codigoDe(assinado.error()), QStringLiteral("assinatura.material_invalido"));
    }

    // DA-2.40 (adendo do CISO): vermelhos antes do código.
    void atributoXmlDaRaizEntraNaCanonicalizacao() {
        // xml:lang na raiz é herdado pelo infDPS na C14N inclusiva (W3C 4.4.1). A cópia atual não o leva; o xmlsec1 reprova.
        QByteArray xml = dpsDeReferencia();
        xml.replace("<DPS xmlns=", "<DPS xml:lang=\"pt-BR\" xmlns=");
        auto assinado = assinarComCredencial(xml, parametrosDaDistro());
        QVERIFY2(assinado.has_value(), "a DPS com xml:lang na raiz tem de assinar");
        QVERIFY2(verificarComXmlsec1(*assinado), "o xmlsec1 tem de verificar: o xml:lang da raiz entra no digest");
    }

    // NL-2.9, condição (a): a expectativa sai da tabela. Trocar a linha da distro na tabela muda o resultado, e a distro
    // sem linha não tem expectativa. Usa uma tabela temporária, e restaura a variável no fim.
    void distroRecusaSha1VemDaTabelaNaoDoCodigo() {
        const QString id = campoDoOsRelease(QStringLiteral("ID"));
        const QString versao = campoDoOsRelease(QStringLiteral("VERSION_ID"));
        QVERIFY2(!id.isEmpty(), "/etc/os-release sem ID");
        QTemporaryDir pasta;
        QVERIFY2(pasta.isValid(), "a pasta temporária da tabela tem de existir");
        const QByteArray antiga = qgetenv("NL_FIXTURES_XMLDSIG");
        auto gravar = [&](const QString& valor) {
            QFile arquivo(pasta.filePath(QStringLiteral("tabela_sha1_por_distro.txt")));
            if (arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                arquivo.write(QStringLiteral("%1 %2 %3\n").arg(id, versao, valor).toUtf8());
            }
        };
        qputenv("NL_FIXTURES_XMLDSIG", pasta.path().toUtf8());
        gravar(QStringLiteral("sha1"));
        const auto comSha1 = recusaSha1SegundoTabela(id, versao);
        gravar(QStringLiteral("algoritmo_bloqueado"));
        const auto comBloqueio = recusaSha1SegundoTabela(id, versao);
        qputenv("NL_FIXTURES_XMLDSIG", antiga);
        QVERIFY2(comSha1.has_value() && comBloqueio.has_value(), "a distro atual tem de ter linha na tabela temporária");
        QVERIFY2(!*comSha1, "linha sha1 na tabela: a suíte assina SHA-1");
        QVERIFY2(*comBloqueio, "linha algoritmo_bloqueado na tabela: a suíte usa SHA-256");
        QVERIFY2(!recusaSha1SegundoTabela(QStringLiteral("distro-inexistente"), QStringLiteral("0")).has_value(),
                 "distro fora da tabela não tem expectativa");
    }

    // NL-2.9, condição (a): a suíte reprova se a distro atual não tem linha na tabela versionada.
    void distroAtualTemLinhaNaTabela() {
        const QString id = campoDoOsRelease(QStringLiteral("ID"));
        const QString versao = campoDoOsRelease(QStringLiteral("VERSION_ID"));
        QVERIFY2(recusaSha1SegundoTabela(id, versao).has_value(),
                 "distro fora de tests/fixtures/xmldsig/tabela_sha1_por_distro.txt: a suíte reprova");
    }

    void tabelaPorDistroVemDoArquivo() {
        // DA-2.40 (adendo): a expectativa de SHA-1 da distro vem da tabela versionada, não de código. Este slot amarra as duas.
        const QByteArray tabela = lerArquivo(qEnvironmentVariable("NL_FIXTURES_XMLDSIG") + QStringLiteral("/tabela_sha1_por_distro.txt"));
        QVERIFY2(!tabela.isEmpty(), "a tabela de expectativa por distro tem de existir");
        const QString id = campoDoOsRelease(QStringLiteral("ID"));
        const QString versao = campoDoOsRelease(QStringLiteral("VERSION_ID"));
        QString esperado;
        for (const QString& linha : QString::fromUtf8(tabela).split(QLatin1Char('\n'))) {
            if (linha.startsWith(QLatin1Char('#')) || linha.trimmed().isEmpty()) {
                continue;
            }
            const QStringList colunas = linha.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (colunas.size() == 3 && colunas[0] == id && colunas[1] == versao) {
                esperado = colunas[2];
            }
        }
        QVERIFY2(!esperado.isEmpty(), "a distro atual tem de ter linha na tabela de expectativa");
        QCOMPARE(distroRecusaSha1(), esperado == QStringLiteral("algoritmo_bloqueado"));
    }

    void contextoProprioIgnoraConfiguracaoGlobal() {
        // DA-2.40 (adendo): roda em processo próprio do CTest, com OPENSSL_CONF do arquivo de teste (que tira SHA-256 do
        // contexto global). O material vem de PEM, por contexto OpenSSL só do teste, nunca pelo PKCS#12.
        using MdPtr = std::unique_ptr<EVP_MD, decltype(&EVP_MD_free)>;
        MdPtr mdGlobal(EVP_MD_fetch(nullptr, "SHA256", nullptr), &EVP_MD_free);
        QVERIFY2(!mdGlobal, "a configuração de teste tem de deixar o contexto global sem SHA-256");

        std::unique_ptr<OSSL_LIB_CTX, decltype(&OSSL_LIB_CTX_free)> ctxTeste(OSSL_LIB_CTX_new(), &OSSL_LIB_CTX_free);
        QVERIFY2(ctxTeste != nullptr, "o contexto do teste tem de abrir");
        std::unique_ptr<OSSL_PROVIDER, decltype(&OSSL_PROVIDER_unload)> provedor(
            OSSL_PROVIDER_load(ctxTeste.get(), "default"), &OSSL_PROVIDER_unload);
        QVERIFY2(provedor != nullptr, "o provider default tem de carregar no contexto do teste");

        const QByteArray chavePem = lerArquivo(caminhoFixture("cnpj_ok.key.pem"));
        const QByteArray certPem = lerArquivo(caminhoFixture("cnpj_ok.pem"));
        QVERIFY2(!chavePem.isEmpty() && !certPem.isEmpty(), "as fixtures PEM da folha cnpj_ok têm de existir");
        std::unique_ptr<BIO, decltype(&BIO_free)> bioChave(BIO_new_mem_buf(chavePem.constData(), chavePem.size()), &BIO_free);
        std::unique_ptr<BIO, decltype(&BIO_free)> bioCert(BIO_new_mem_buf(certPem.constData(), certPem.size()), &BIO_free);
        std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> chave(
            PEM_read_bio_PrivateKey_ex(bioChave.get(), nullptr, nullptr, nullptr, ctxTeste.get(), nullptr), &EVP_PKEY_free);
        std::unique_ptr<X509, decltype(&X509_free)> cert(PEM_read_bio_X509_AUX(bioCert.get(), nullptr, nullptr, nullptr), &X509_free);
        QVERIFY2(chave != nullptr && cert != nullptr, "o PEM da folha tem de ler no contexto do teste");

        xd::MaterialAssinatura material{cert.get(), chave.get()};
        auto assinado = xd::AssinadorXml::assinarEnveloped(dpsDeReferencia(),
            QStringLiteral("DPS430510821122233300018100001000000000000001"), parametrosSha256(), material);
        QVERIFY2(assinado.has_value(), "o contexto próprio do assinador tem de assinar mesmo com o global restrito");
    }

    void assinaSha1NoSdkKdeSemVariavel() {
        // Roda só no build Flatpak (SDK KDE 6.11, runtime medido na NL-2.3, passo 0). A variável tem de estar ausente.
        // DA-2.40 (b): imprime o estado da variável, para o log do job mostrar que ela não existe no SDK.
        const bool variavelPresente = qEnvironmentVariableIsSet("OPENSSL_ENABLE_SHA1_SIGNATURES");
        qInfo("VARIAVEL_SHA1_PRESENTE=%d", int(variavelPresente));
        QVERIFY2(!variavelPresente, "a variável de SHA-1 não pode existir no processo do app");
        auto assinado = assinarComCredencial(dpsDeReferencia(), parametrosSha1());
        QVERIFY2(assinado.has_value(), "o SDK KDE 6.11 sem a variável tem de assinar SHA-1");
    }
};

QTEST_APPLESS_MAIN(TesteAssinadorXml)
#include "tst_assinador_xml.moc"
