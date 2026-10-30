// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do ClienteSefin, da política de retry, do catálogo de erros e do leitor de erro (NL-2.8). Contra o servidor falso
// (tools/servidor_falso, pelo lancador, com oráculo): cada slot sobe o servidor como processo próprio.
// A sequência de requisições sai do Log do próprio cliente (evento sefin.requisicao, com metodo e status), sem ler o
// servidor. Slot por CTest (mid.sefin.cliente.<nome>). Falha é QVERIFY (L-35).
#include <nl/back/dps/dps_assinada.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/back/dps/id_dps.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/ports/gateway_sefin.h>

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>
#include <nl/mid/sefin/cliente/cliente_sefin.h>
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QProcess>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace std::chrono_literals;
namespace bk = nl::back;
namespace fl = nl::foundation::log;
namespace fp = nl::foundation::pkcs12;
namespace ft = nl::foundation::transporte;
namespace mc = nl::mid::sefin::cliente;

namespace {

QByteArray lerFixture(const QString& nome)
{
    QFile arquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QLatin1Char('/') + nome);
    return arquivo.open(QIODevice::ReadOnly) ? arquivo.readAll() : QByteArray();
}

// Credencial cnpj_ok (NL-2.2), com a mesma cadeia que o app usa.
std::expected<fp::CredencialX509, fp::ErroCredencial> credencial()
{
    auto ancoras = fp::Ancoras::dePem(lerFixture(QStringLiteral("ac.pem")));
    if (!ancoras) {
        return std::unexpected(fp::ErroCredencial::CadeiaInvalida);
    }
    QFile senhaArquivo(qEnvironmentVariable("NL_CERTIFICADOS_DIR") + QStringLiteral("/senha_teste.txt"));
    fp::SenhaSegura senha = fp::SenhaSegura::de(senhaArquivo.open(QIODevice::ReadOnly)
                                                    ? QByteArray(senhaArquivo.readLine().trimmed())
                                                    : QByteArray());
    fp::CarregadorPkcs12 carregador;
    return carregador.carregar(lerFixture(QStringLiteral("cnpj_ok.pfx")), senha, *ancoras);
}

// TLS de cliente a partir da credencial: certificado DER e chave PKCS#8 PEM, como o transporte da NL-2.5 espera.
ft::ConfiguracaoTls configuracaoTls(const fp::CredencialX509& cred)
{
    ft::ConfiguracaoTls tls;
    unsigned char* der = nullptr;
    const int tamanho = i2d_X509(cred.certificado(), &der);
    if (tamanho > 0 && der != nullptr) {
        tls.certificadoCliente = QSslCertificate(QByteArray(reinterpret_cast<const char*>(der), tamanho), QSsl::Der);
    }
    OPENSSL_free(der);
    BIO* saida = BIO_new(BIO_s_mem());
    if (saida != nullptr && PEM_write_bio_PKCS8PrivateKey(saida, cred.chavePrivada(), nullptr, nullptr, 0, nullptr,
                                                          nullptr) == 1) {
        char* dados = nullptr;
        const long n = BIO_get_mem_data(saida, &dados);
        tls.chaveCliente = QSslKey(QByteArray(dados, static_cast<qsizetype>(n)), QSsl::Rsa, QSsl::Pem);
    }
    BIO_free(saida);
    tls.autoridades = QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem);
    return tls;
}

// DPS de referência assinada pelo xmlsec1 (SHA-256, quatro imagens). TROCAR pelo AssinadorXml::assinarEnveloped quando a NL-2.3 entrar na onda-2.
std::expected<bk::DpsAssinada, std::string> dpsAssinada(const fp::CredencialX509& cred)
{
    QFile arquivo(qEnvironmentVariable("NL_FIXTURES_DPS") + QStringLiteral("/valida_dps_mei_minima.xml"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(std::string("fixture da DPS"));
    }
    const QByteArray base = arquivo.readAll();
    const QString id = QStringLiteral("DPS430510821122233300018100001000000000000001");
    const QByteArray assinatura =
        QByteArrayLiteral("<Signature xmlns=\"http://www.w3.org/2000/09/xmldsig#\"><SignedInfo>"
                          "<CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/>"
                          "<SignatureMethod Algorithm=\"http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\"/>"
                          "<Reference URI=\"#") + id.toUtf8() +
        QByteArrayLiteral("\"><Transforms>"
                          "<Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/>"
                          "<Transform Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/></Transforms>"
                          "<DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"/>"
                          "<DigestValue></DigestValue></Reference></SignedInfo><SignatureValue></SignatureValue>"
                          "<KeyInfo><X509Data></X509Data></KeyInfo></Signature>");
    const int fim = base.lastIndexOf("</DPS>");
    if (fim < 0) {
        return std::unexpected(std::string("fixture sem </DPS>"));
    }
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return std::unexpected(std::string("sem pasta temporária"));
    }
    const QString chavePem = dir.filePath(QStringLiteral("chave.pem"));
    const QString certPem = dir.filePath(QStringLiteral("cert.pem"));
    const QString modelo = dir.filePath(QStringLiteral("modelo.xml"));
    const QString saida = dir.filePath(QStringLiteral("assinado.xml"));
    BIO* bioChave = BIO_new(BIO_s_mem());
    BIO* bioCert = BIO_new(BIO_s_mem());
    if (bioChave == nullptr || bioCert == nullptr ||
        PEM_write_bio_PKCS8PrivateKey(bioChave, cred.chavePrivada(), nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
        PEM_write_bio_X509(bioCert, cred.certificado()) != 1) {
        BIO_free(bioChave);
        BIO_free(bioCert);
        return std::unexpected(std::string("material PEM"));
    }
    char* dadosChave = nullptr;
    char* dadosCert = nullptr;
    const long nChave = BIO_get_mem_data(bioChave, &dadosChave);
    const long nCert = BIO_get_mem_data(bioCert, &dadosCert);
    QFile fChave(chavePem);
    QFile fCert(certPem);
    QFile fModelo(modelo);
    if (!fChave.open(QIODevice::WriteOnly) || !fCert.open(QIODevice::WriteOnly) || !fModelo.open(QIODevice::WriteOnly)) {
        BIO_free(bioChave);
        BIO_free(bioCert);
        return std::unexpected(std::string("gravação dos arquivos de teste"));
    }
    fChave.write(dadosChave, nChave);
    fCert.write(dadosCert, nCert);
    fModelo.write(base.left(fim) + assinatura + base.mid(fim));
    fChave.close();
    fCert.close();
    fModelo.close();
    BIO_free(bioChave);
    BIO_free(bioCert);

    QProcess versao;
    versao.start(qEnvironmentVariable("NL_XMLSEC1"), {QStringLiteral("--version")});
    versao.waitForFinished(30000);
    const bool pelo13 = QRegularExpression(QStringLiteral("xmlsec1 1\\.(\\d+)"))
                            .match(QString::fromUtf8(versao.readAllStandardOutput() + versao.readAllStandardError()))
                            .captured(1).toInt() >= 3;
    QStringList args{QStringLiteral("--sign")};
    if (pelo13) {
        args << QStringLiteral("--lax-key-search");
    }
    args << QStringLiteral("--id-attr:Id") << QStringLiteral("infDPS") << QStringLiteral("--privkey-pem")
         << (chavePem + QLatin1Char(',') + certPem) << QStringLiteral("--output") << saida << modelo;
    QProcess xmlsec;
    xmlsec.start(qEnvironmentVariable("NL_XMLSEC1"), args);
    if (!xmlsec.waitForFinished(120000) || xmlsec.exitCode() != 0) {
        return std::unexpected(std::string("xmlsec1 não assinou a DPS de referência"));
    }
    QFile lido(saida);
    if (!lido.open(QIODevice::ReadOnly)) {
        return std::unexpected(std::string("saída do xmlsec1 ausente"));
    }
    auto municipio = bk::CodigoMunicipio::de(QStringLiteral("4305108"));
    auto cnpj = bk::Cnpj::de(QStringLiteral("11222333000181"));
    auto serie = bk::Serie::de(1);
    auto numero = bk::NumeroDps::de(std::int64_t{1});
    if (!municipio || !cnpj || !serie || !numero) {
        return std::unexpected(std::string("identificadores"));
    }
    auto idDps = bk::IdDps::montar(*municipio, bk::Inscricao{*cnpj}, *serie, *numero);
    if (!idDps) {
        return std::unexpected(std::string("id"));
    }
    return bk::DpsAssinada{*idDps, lido.readAll()};
}

// Servidor falso em processo próprio, com oráculo (--com-oraculo), e as falhas do roteiro. Fecha o stdin ao sair.
class ServidorDeTeste {
public:
    explicit ServidorDeTeste(const QStringList& falhas)
    {
        QStringList argumentos{QStringLiteral("-I"), qEnvironmentVariable("NL_LANCADOR"),
                               qEnvironmentVariable("NL_CERTIFICADOS_DIR"), QStringLiteral("--com-oraculo")};
        for (const QString& falha : falhas) {
            argumentos << QStringLiteral("--falha") << falha.split(QLatin1Char(' '));
        }
        processo_.start(qEnvironmentVariable("NL_PYTHON3"), argumentos);
        if (processo_.waitForReadyRead(20000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
        if (porta_ == 0) {
            diagnostico_ = QStringLiteral("servidor falso não subiu; stderr: ") +
                           QString::fromUtf8(processo_.readAllStandardError());
        }
    }
    ~ServidorDeTeste()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
    }
    int porta() const { return porta_; }
    const QString& diagnostico() const { return diagnostico_; }

private:
    QProcess processo_;
    int porta_ = 0;
    QString diagnostico_;
};

// Sequência de requisições do cliente, lida do Log: "METODO:status" por linha de evento sefin.requisicao.
std::vector<std::string> sequencia(const std::string& saida)
{
    std::vector<std::string> itens;
    std::istringstream linhas(saida);
    std::string linha;
    while (std::getline(linhas, linha)) {
        if (linha.find("\"evento\":\"sefin.requisicao\"") == std::string::npos) {
            continue;
        }
        const auto m = linha.find("\"metodo\":\"");
        const auto s = linha.find("\"status\":\"");
        if (m == std::string::npos || s == std::string::npos) {
            continue;
        }
        const auto metodo = linha.substr(m + 10, linha.find('"', m + 10) - (m + 10));
        const auto status = linha.substr(s + 10, linha.find('"', s + 10) - (s + 10));
        itens.push_back(metodo + ":" + status);
    }
    return itens;
}

struct Montagem {
    std::ostringstream destino;
    std::unique_ptr<fl::Log> log;
    std::unique_ptr<ft::TransporteHttps> transporte;
    std::unique_ptr<mc::ClienteSefin> cliente;
};

std::unique_ptr<Montagem> montar(int porta, const fp::CredencialX509& cred)
{
    auto m = std::make_unique<Montagem>();
    m->log = std::make_unique<fl::Log>(
        m->destino, "sefin",
        std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                         {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
        std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro"});
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, configuracaoTls(cred));
    mc::PoliticaRetry politica;
    politica.base = 1ms;  // a espera é real mas curta: o teste mede a ordem, não o tempo
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica);
    return m;
}

} // namespace

class TesteClienteSefin : public QObject {
    Q_OBJECT

private slots:
    void sucessoExtraiChaveDe50Posicoes()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "a Sefin de teste tem de aceitar a DPS assinada");
        QCOMPARE(nota->chave.texto().size(), qsizetype{50});
        QVERIFY(!nota->xmlNfse.isEmpty());
    }

    void postTransitorioConsultaDpsAntesDeReenviar()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "depois do 5xx, o cliente consulta e reenvia até aceitar");
        const auto itens = sequencia(m->destino.str());
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:201"};
        QVERIFY2(itens == esperado, "o POST nunca repete sozinho: consulta GET /dps antes do reenvio");
    }

    void dps404E2404LiberaReenvio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        QVERIFY2(m->cliente->emitir(*dps).has_value(), "com E2404 na consulta, o reenvio libera a emissão");
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("\"codigoE\":\"E2404\"") != std::string::npos,
                 "a consulta tem de registrar o código E2404 (só o código, nada mais)");
    }

    void conflito409ConsultaENaoReenvia()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse conflito409")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "409 não pode virar nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::DpsDuplicada, "409 tem de ser DpsDuplicada");
        const std::vector<std::string> esperado{"POST:409", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "409 consulta a DPS e nunca reenvia o POST");
    }

    void rejeicaoE0014NaoRepete()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "rejeição não pode virar nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Rejeitada, "código E tem de ser Rejeitada");
        QCOMPARE(QString::fromStdString(resultado.error().codigoE), QStringLiteral("E0014"));
        const std::vector<std::string> esperado{"POST:400"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "rejeição com código E nunca repete");
    }

    void certificadoRevogadoMapeadoPelaRejeicaoDaSefin()
    {
        // Revogação fora da v1 (decisão do líder, 29/10/2026): a recusa da Sefin com E1207 vira a mensagem do catálogo.
        // FATO (Anexo I de 20/10/2026, sharedStrings: "E1207" seguido de "Certificado de Transmissão revogado"). A linha
        // exata na aba RN é INFERÊNCIA, a conferir na NL-2.10.
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E1207")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "certificado revogado não pode emitir");
        QCOMPARE(QString::fromStdString(resultado.error().codigoE), QStringLiteral("E1207"));
        QCOMPARE(QString::fromStdString(resultado.error().mensagem),
                 QStringLiteral("Certificado de Transmissão revogado"));
    }

    void textoLivreDaApiComCpfSoLogaCodigoE()
    {
        // ADR-0005 (c): o texto livre da API nunca entra no log nem na mensagem. O servidor põe um CPF no texto livre.
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("12345678909") == std::string::npos, "o CPF do texto livre não pode aparecer no log");
        QVERIFY2(saida.find("\"codigoE\":\"E0014\"") != std::string::npos, "o log tem de trazer o código E");
        QVERIFY2(resultado.error().mensagem.find("12345678909") == std::string::npos,
                 "a mensagem do erro não pode trazer o texto livre");
    }

    void getTransitorioRepeteComBackoff()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto consulta = m->cliente->consultarDps(dps->id);
        QVERIFY2(!consulta.has_value(), "sem DPS gerada, a consulta devolve erro, não chave");
        QVERIFY2(consulta.error().motivo == bk::MotivoFalhaSefin::DpsNaoGerada, "E2404 tem de ser DpsNaoGerada");
        const std::vector<std::string> esperado{"GET:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "GET repete com backoff depois do 5xx");
    }

    void leitorAceitaErroEErrosECodigoECodigo()
    {
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"erro\":{\"Codigo\":\"E0014\",\"Descricao\":\"x\"}}")) ==
                std::optional<std::string>{"E0014"});
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"erros\":[{\"codigo\":\"E1207\"}]}")) ==
                std::optional<std::string>{"E1207"});
        QVERIFY(mc::codigoDoErro(QByteArrayLiteral("{\"Codigo\":\"E2404\"}")) == std::optional<std::string>{"E2404"});
        QVERIFY(!mc::codigoDoErro(QByteArrayLiteral("{\"sem\":\"codigo\"}")).has_value());
        QVERIFY(!mc::codigoDoErro(QByteArrayLiteral("não é json")).has_value());
    }

    void catalogoDaMensagemDoCodigoE()
    {
        QVERIFY(mc::CatalogoErrosSefin::conhece("E1207"));
        QCOMPARE(QString::fromUtf8(mc::CatalogoErrosSefin::mensagem("E1207").data()),
                 QStringLiteral("Certificado de Transmissão revogado"));
        QVERIFY2(!mc::CatalogoErrosSefin::conhece("E9999"), "código fora do catálogo não é conhecido");
    }

    void xmlNfseGuardaXmlDescomprimido()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "a Sefin de teste tem de aceitar a DPS assinada");
        QVERIFY2(!nota->xmlPendente, "o XML de um sucesso legível não fica pendente");
        QVERIFY2(nota->xmlNfse == dps->xml, "xmlNfse guarda o XML descompactado, não o GZip nem o Base64");
    }

    void base64InvalidoRecusado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_base64_invalido")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com caractere fora do Base64 devolve a chave, sem falha");
        QVERIFY2(nota->xmlPendente, "caractere fora do Base64 marca o XML como pendente");
        QVERIFY2(nota->xmlNfse.isEmpty(), "XML pendente não traz bytes");
        QCOMPARE(nota->chave.texto().size(), qsizetype{50});
    }

    void xmlIlegivelNumSucessoNaoLiberaReenvio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        const QStringList roteiros{QStringLiteral("POST /nfse nfse_gzip_invalido"),
                                   QStringLiteral("POST /nfse nfse_acima_do_teto")};
        for (const QString& roteiro : roteiros) {
            ServidorDeTeste srv({roteiro});
            QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
            auto m = montar(srv.porta(), *cred);
            auto nota = m->cliente->emitir(*dps);
            QVERIFY2(nota.has_value(), "sucesso com XML ilegível devolve a chave, sem falha");
            QVERIFY2(nota->xmlPendente, "XML que não descompacta fica pendente");
            QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"POST:201"},
                     "XML ilegível num sucesso nunca libera reenvio: um POST e nenhuma consulta");
        }
    }

    void backoffDobraAteAPrimeiraRepeticao()
    {
        mc::PoliticaRetry politica;
        politica.base = 500ms;
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(1).count()), qint64{0});
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(2).count()), qint64{500});
        QCOMPARE(static_cast<qint64>(politica.esperaAntesDaTentativa(3).count()), qint64{1000});
    }

    void backoffNaoEstouraComExpoenteAlto()
    {
        // DA-2.41 (d): 1 << (n - 2) com n de 64 ou 1000 é comportamento indefinido. O expoente tem teto explícito.
        mc::PoliticaRetry politica;
        politica.base = 500ms;
        const auto saturado = politica.esperaAntesDaTentativa(12).count();
        QVERIFY2(saturado > 0, "a espera saturada tem de ser positiva");
        QCOMPARE(politica.esperaAntesDaTentativa(64).count(), saturado);
        QCOMPARE(politica.esperaAntesDaTentativa(1000).count(), saturado);
        for (int n = 2; n < 70; ++n) {
            QVERIFY2(politica.esperaAntesDaTentativa(n).count() <= politica.esperaAntesDaTentativa(n + 1).count(),
                     "a espera não pode diminuir com o número da tentativa");
        }
    }

    // DA-2.41 (a): POST com 2xx sem chave consulta GET /dps e devolve a chave da consulta, nunca Definitiva.
    void posPost2xxSemChaveConsultaDps()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse post_sem_chave")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "201 sem chave consulta GET /dps e devolve a chave da consulta");
        const std::vector<std::string> esperado{"POST:201", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "2xx sem chave consulta GET /dps antes de responder");
    }

    void posPost2xxForaDe200e201ConsultaDps()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse status202")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "202 consulta GET /dps e devolve a chave da consulta");
        const std::vector<std::string> esperado{"POST:202", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "2xx fora de 200 e 201 consulta GET /dps");
    }

    // DA-2.41 (a): 3xx no POST é estado indeterminado. Consulta antes de qualquer reenvio.
    void post3xxConsultaDpsAntesDeReenviar()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse redireciona_post")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "depois do 3xx, a consulta mostra que a DPS não gerou nota e o reenvio emite");
        const std::vector<std::string> esperado{"GET:404", "POST:201"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "3xx no POST consulta GET /dps antes de reenviar");
    }

    // DA-2.41 (a): erro definitivo depois do envio é indeterminado, e a consulta que não diz o estado também.
    void erroDefinitivoDepoisDoEnvioViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse post_ce_gzip"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_outro")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem resposta definitiva, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "erro depois do envio, com consulta sem resposta, é Indeterminado");
        QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"GET:404"}, "a consulta é feita");
    }

    // DA-2.41 (a): qualquer resultado da consulta diferente de chave ou E2404 é Indeterminado, nunca Rejeitada.
    void consultaComCodigoQueNaoE2404ViraIndeterminado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"),
                             QStringLiteral("GET /dps/DPS430510821122233300018100001000000000000001 consulta_404_outro")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY2(!resultado.has_value(), "sem chave e sem E2404, não há nota");
        QVERIFY2(resultado.error().motivo == bk::MotivoFalhaSefin::Indeterminado,
                 "consulta com código diferente de E2404 é Indeterminado, nunca Rejeitada");
        const std::vector<std::string> esperado{"POST:503", "GET:404"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "a consulta vem antes de qualquer decisão");
    }

    // DA-2.41 (a): E2404 libera o reenvio da mesma DPS (mesmo Id, mesmo número, por construção do envelope).
    void naoGeradaReenviaMesmoIdNuncaNumeroNovo()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        // Dupla emissão (o pior defeito fiscal): o servidor registra o Id e o nDPS de cada POST que chegou. Os dois
        // envios têm de ser a mesma DPS. Este teste fica vermelho se o cliente remontar o envelope com outro número.
        auto registro = m->transporte->enviar(
            ft::RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/_teste/posts"), {}, {}});
        QVERIFY2(registro.has_value(), "o registro de POSTs do servidor falso tem de responder");
        const QJsonArray envios = QJsonDocument::fromJson(registro->corpo).array();
        QCOMPARE(envios.size(), qsizetype{2});
        const QJsonObject primeiro = envios.at(0).toObject();
        const QJsonObject segundo = envios.at(1).toObject();
        QVERIFY2(!primeiro.value(QStringLiteral("nDPS")).toString().isEmpty(), "o primeiro POST tem de trazer o nDPS");
        QVERIFY2(primeiro.value(QStringLiteral("id")).toString() == segundo.value(QStringLiteral("id")).toString(),
                 "o reenvio tem de ter o mesmo Id da DPS");
        QVERIFY2(primeiro.value(QStringLiteral("nDPS")).toString() == segundo.value(QStringLiteral("nDPS")).toString(),
                 "o reenvio tem de ter o mesmo número (nDPS): remontar com outro número é dupla emissão");
        QVERIFY2(nota.has_value(), "E2404 na consulta libera o reenvio da mesma DPS");
        // O reenvio reaproveita o envelope e o Id da DPS montada uma vez: a sequência tem um POST a mais, e nenhum GET de nota.
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:201"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "o reenvio segue a consulta, sem outra tentativa de número");
    }

    // DA-2.41 (a): E0014 num reenvio leva à consulta, e a consulta devolve a chave (a nota já existia).
    void e0014NoReenvioConsultaEDevolveAChave()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse erro5xx"), QStringLiteral("POST /nfse rejeicao_gerada:E0014")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "E0014 num reenvio consulta GET /dps e devolve a chave, nunca rejeição");
        const std::vector<std::string> esperado{"POST:503", "GET:404", "POST:400", "GET:200", "GET:200"};
        QVERIFY2(sequencia(m->destino.str()) == esperado, "E0014 no reenvio vai para a consulta");
    }

    // DA-2.41 (a): falha antes do envio é Transitoria, sem consulta. O handshake sem certificado é antes do envio.
    void falhaAntesDoEnvioETransitoriaSemConsulta()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        auto recusada = montar(1, *cred);  // porta 1: ninguém escuta, a conexão é recusada antes do envio
        auto semConexao = recusada->cliente->emitir(*dps);
        QVERIFY2(!semConexao.has_value(), "sem conexão não há nota");
        QVERIFY2(semConexao.error().motivo == bk::MotivoFalhaSefin::Transitoria, "conexão recusada é Transitoria");
        QVERIFY2(sequencia(recusada->destino.str()).empty(), "conexão recusada não consulta a DPS");

        ServidorDeTeste srv({});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        fl::Log registro(recusada->destino, "sefin",
                         std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                                          {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
                         std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro"});
        ft::ConfiguracaoTls semCertificado = configuracaoTls(*cred);
        semCertificado.certificadoCliente = QSslCertificate();
        semCertificado.chaveCliente = QSslKey();
        const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(srv.porta())), QString(), 5000ms};
        ft::TransporteHttps transporte(base, semCertificado);
        mc::PoliticaRetry politica;
        politica.base = 1ms;
        mc::ClienteSefin cliente(transporte, registro, politica);
        auto tls = cliente.emitir(*dps);
        QVERIFY2(!tls.has_value(), "sem certificado de cliente, o servidor recusa o handshake");
        QVERIFY2(tls.error().motivo == bk::MotivoFalhaSefin::Transitoria,
                 "handshake recusado é antes do envio: Transitoria, sem consulta");
        QVERIFY2(sequencia(recusada->destino.str()).empty(), "handshake recusado não consulta a DPS");
    }

    // DA-2.41 (b): código E fora de ^E\d{3,4}$ fica vazio, e o log recebe só o marcador codigo_invalido.
    void codigoEComCpfNaoChegaAoLog()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_bruta:E1203_CPF_12345678909")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        const std::string saida = m->destino.str();
        QVERIFY2(saida.find("sefin.requisicao") != std::string::npos, "piso: a varredura tem de ler um log não vazio");
        QVERIFY2(saida.find("12345678909") == std::string::npos, "o CPF do campo Codigo não pode aparecer no log");
        QVERIFY2(saida.find("codigo_invalido") != std::string::npos, "o log recebe só o marcador codigo_invalido");
        QVERIFY2(resultado.error().codigoE.empty(), "código fora do formato fica vazio no erro");
        QVERIFY2(resultado.error().mensagem.find("12345678909") == std::string::npos,
                 "a mensagem do erro não pode trazer o CPF");
    }

    void codigoEForaDoFormatoFicaVazio()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse rejeicao_bruta:E12345")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto resultado = m->cliente->emitir(*dps);
        QVERIFY(!resultado.has_value());
        QVERIFY2(resultado.error().codigoE.empty(), "E12345 tem cinco dígitos: fora de ^E\\d{3,4}$, fica vazio");
        QVERIFY2(m->destino.str().find("codigo_invalido") != std::string::npos, "o log marca codigo_invalido");
    }

    // DA-2.41 (c): Base64 com caractere fora do alfabeto e tamanho válido. Só a checagem de alfabeto recusa.
    void base64ComCaractereInvalidoETamanhoValidoRecusado()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_base64_caractere_invalido")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com caractere fora do Base64 devolve a chave");
        QVERIFY2(nota->xmlPendente, "caractere fora do alfabeto marca o XML como pendente, mesmo com tamanho válido");
        QVERIFY2(nota->xmlNfse.isEmpty(), "XML pendente não traz bytes");
    }

    // DA-2.41 (d): o segundo membro do gzip é erro explícito e leva ao XML pendente, sem liberar reenvio.
    void membroExtraDoGzipViraXmlPendente()
    {
        auto cred = credencial();
        QVERIFY2(cred.has_value(), "a fixture cnpj_ok tem de carregar");
        auto dps = dpsAssinada(*cred);
        QVERIFY2(dps.has_value(), "a DPS de referência tem de assinar");
        ServidorDeTeste srv({QStringLiteral("POST /nfse nfse_gzip_dois_membros")});
        QVERIFY2(srv.porta() > 0, qPrintable(srv.diagnostico()));
        auto m = montar(srv.porta(), *cred);
        auto nota = m->cliente->emitir(*dps);
        QVERIFY2(nota.has_value(), "sucesso com segundo membro gzip devolve a chave");
        QVERIFY2(nota->xmlPendente, "o segundo membro gzip faz o XML ficar pendente, sem XML falso");
        QVERIFY2(sequencia(m->destino.str()) == std::vector<std::string>{"POST:201"}, "sem reenvio");
    }
};

// QTEST_GUILESS_MAIN cria o QCoreApplication: o transporte espera pelo QEventLoop e o seu tempo limite é um QTimer, e
// nenhum dos dois funciona sem laço de eventos (com APPLESS, o envio ficava sem prazo até o timeout de 300 s do QTest).
QTEST_GUILESS_MAIN(TesteClienteSefin)
#include "tst_cliente_sefin.moc"
