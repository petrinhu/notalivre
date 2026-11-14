// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Montagem de teste do ClienteSefin contra o servidor falso (NL-3.C; S5b, DA-3C.22, decisão do líder de 14/11/2026). Corpo
// movido byte a byte de tst_cliente_sefin.cpp. Usado por tst_cliente_sefin e por tst_cliente_sefin_falhas_envio, dois
// executáveis: o namespace anônimo é de propósito, cada um tem a própria cópia.
#ifndef NL_TESTE_APOIO_CLIENTE_SEFIN_H
#define NL_TESTE_APOIO_CLIENTE_SEFIN_H

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

#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>
#include <nl/mid/sefin/cliente/comparador_infdps_c14n.h>
#include <nl/mid/sefin/cliente/cliente_sefin.h>
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
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
#include <QTcpServer>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QTcpSocket>
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
    explicit ServidorDeTeste(const QStringList& falhas, const QString& tlsMaximo = {})
    {
        QStringList argumentos{QStringLiteral("-I"), qEnvironmentVariable("NL_LANCADOR"),
                               qEnvironmentVariable("NL_CERTIFICADOS_DIR"), QStringLiteral("--com-oraculo")};
        for (const QString& falha : falhas) {
            argumentos << QStringLiteral("--falha") << falha.split(QLatin1Char(' '));
        }
        if (!tlsMaximo.isEmpty()) {
            argumentos << QStringLiteral("--tls-maximo") << tlsMaximo;  // versão máxima de TLS do cenário (rodada 4)
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

// Comparador de teste (adendo da DA-2.41): devolve o resultado configurado e guarda o que recebeu.
class ComparadorFalso final : public mc::ComparadorInfDps {
public:
    explicit ComparadorFalso(mc::ResultadoComparacaoInfDps resultado) : resultado_(resultado) {}
    mc::ResultadoComparacaoInfDps comparar(const bk::IdDps&, const QByteArray& dpsXml,
                                           const QByteArray& nfseXml) const override
    {
        chamadas_++;
        dpsRecebido_ = dpsXml;
        nfseRecebido_ = nfseXml;
        return resultado_;
    }
    mutable int chamadas_ = 0;
    mutable QByteArray dpsRecebido_;
    mutable QByteArray nfseRecebido_;

private:
    mc::ResultadoComparacaoInfDps resultado_;
};

// Observador de teste (NL-2.10c, passo 1): guarda as respostas HTTP que o cliente reporta, na ordem em que chegaram.
class RegistroObservador final : public mc::ObservadorRespostas {
public:
    struct Resposta {
        std::string metodo;
        std::string rota;
        int status = 0;
        QByteArray tipoConteudo;
        QByteArray corpo;
    };
    void registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                   const QByteArray& corpo) override
    {
        respostas.push_back(Resposta{std::string(metodo), std::string(rota), status, tipoConteudo, corpo});
    }
    std::vector<Resposta> respostas;
};

struct Montagem {
    std::ostringstream destino;
    std::unique_ptr<fl::Log> log;
    std::unique_ptr<ft::TransporteHttps> transporte;
    std::unique_ptr<mc::ClienteSefin> cliente;
};

std::unique_ptr<Montagem> montar(int porta, const fp::CredencialX509& cred,
                                 std::shared_ptr<const mc::ComparadorInfDps> comparador = nullptr,
                                 mc::ObservadorRespostas* observador = nullptr)
{
    auto m = std::make_unique<Montagem>();
    m->log = std::make_unique<fl::Log>(
        m->destino, "sefin",
        std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                         {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
        std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro", "sefin.transporte_falhou"});
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, configuracaoTls(cred));
    mc::PoliticaRetry politica;
    politica.base = 1ms;  // a espera é real mas curta: o teste mede a ordem, não o tempo
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica, comparador, observador);
    return m;
}

// Cliente sem certificado de cliente: o servidor falso exige o certificado (CERT_REQUIRED), então a recusa é da camada TLS.
std::unique_ptr<Montagem> montarSemCertificado(int porta)
{
    auto m = std::make_unique<Montagem>();
    m->log = std::make_unique<fl::Log>(
        m->destino, "sefin",
        std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                         {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
        std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro", "sefin.transporte_falhou"});
    ft::ConfiguracaoTls tls;
    tls.autoridades = QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem);
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, tls);
    mc::PoliticaRetry politica;
    politica.base = 1ms;
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica);
    return m;
}

// Quantos POST /nfse o servidor recebeu, pelo registro de teste. Usa um cliente com certificado, que o servidor exige.
int postsNoServidor(int porta, const fp::CredencialX509& cred)
{
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    ft::TransporteHttps transporte(base, configuracaoTls(cred));
    auto resposta = transporte.enviar(
        ft::RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/_teste/posts"), {}, {}});
    if (!resposta) {
        return -1;
    }
    return static_cast<int>(QJsonDocument::fromJson(resposta->corpo).array().size());
}

} // namespace

#endif // NL_TESTE_APOIO_CLIENTE_SEFIN_H
