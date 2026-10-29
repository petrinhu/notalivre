// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de integração do transporte HTTPS com mTLS (NL-2.5), contra o servidor falso (NL-2.6). Cada slot roda por
// nome no CTest (transporte.<slot>). O servidor sobe em processo próprio, pelo lançador Python, com a AC de teste da
// NL-2.1. Os certificados de cliente são lidos dos .pfx da fixture com o OpenSSL, dentro do teste (não pelo motor
// PKCS#12 de produção). Nenhum helper lança exceção: falha vira QVERIFY no teste que chamou (L-35).

#include <nl/foundation/transporte/transporte_https.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/x509.h>

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QProcess>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtTest>

#include <chrono>

using namespace nl::foundation::transporte;
using namespace std::chrono_literals;

namespace {

QString pastaFixtures() {
    return qEnvironmentVariable("NL_CERTIFICADOS_DIR");
}

QByteArray lerFixture(const QString& nome) {
    QFile arquivo(pastaFixtures() + QLatin1Char('/') + nome);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

QByteArray senhaDaFixture() {
    QFile arquivo(pastaFixtures() + QStringLiteral("/senha_teste.txt"));
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readLine().trimmed();
}

// Certificado e chave de cliente de um .pfx da fixture, lidos com o OpenSSL. Vazios se a leitura falhar.
struct ParCliente {
    QSslCertificate certificado;
    QSslKey chave;
};

ParCliente parDoPfx(const QString& nome) {
    ParCliente par;
    const QByteArray pfx = lerFixture(nome);
    const QByteArray senha = senhaDaFixture();
    BIO* bio = BIO_new_mem_buf(pfx.constData(), static_cast<int>(pfx.size()));
    PKCS12* p12 = bio != nullptr ? d2i_PKCS12_bio(bio, nullptr) : nullptr;
    BIO_free(bio);
    if (p12 == nullptr) {
        ERR_clear_error();
        return par;
    }
    EVP_PKEY* chave = nullptr;
    X509* cert = nullptr;
    STACK_OF(X509)* ca = nullptr;
    const int ok = PKCS12_parse(p12, senha.constData(), &chave, &cert, &ca);
    PKCS12_free(p12);
    sk_X509_pop_free(ca, X509_free);
    if (ok == 1 && cert != nullptr && chave != nullptr) {
        unsigned char* der = nullptr;
        const int tamanho = i2d_X509(cert, &der);
        if (tamanho > 0 && der != nullptr) {
            par.certificado = QSslCertificate(QByteArray(reinterpret_cast<const char*>(der), tamanho), QSsl::Der);
        }
        OPENSSL_free(der);
        BIO* saida = BIO_new(BIO_s_mem());
        if (saida != nullptr && PEM_write_bio_PKCS8PrivateKey(saida, chave, nullptr, nullptr, 0, nullptr, nullptr) == 1) {
            char* dados = nullptr;
            const long n = BIO_get_mem_data(saida, &dados);
            par.chave = QSslKey(QByteArray(dados, static_cast<qsizetype>(n)), QSsl::Rsa, QSsl::Pem);
        }
        BIO_free(saida);
    }
    X509_free(cert);
    EVP_PKEY_free(chave);
    return par;
}

QList<QSslCertificate> autoridadeDeTeste() {
    return QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem);
}

UrlBase urlBase(int porta, const QString& prefixo, std::chrono::milliseconds tempoLimite) {
    return UrlBase{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), prefixo, tempoLimite};
}

RequisicaoHttp consultaDps() {
    return RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/dps/ABC"), {}, {}};
}

// Servidor falso em processo próprio. Lê a porta da primeira linha do lançador e o encerra ao sair. Uma falha
// roteirizada é a tripla METODO, CAMINHO, FALHA (um único --falha por tripla, como o lançador espera).
class ServidorDeTeste {
public:
    explicit ServidorDeTeste(const QStringList& falha = {}) {
        QStringList argumentos{QStringLiteral("-I"), QStringLiteral(NL_LANCADOR), pastaFixtures()};
        if (!falha.isEmpty()) {
            argumentos << QStringLiteral("--falha") << falha;
        }
        processo_.start(QStringLiteral(NL_PYTHON3), argumentos);
        if (processo_.waitForReadyRead(15000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
    }

    ~ServidorDeTeste() {
        processo_.closeWriteChannel();
        if (!processo_.waitForFinished(5000)) {
            processo_.kill();
            processo_.waitForFinished(2000);
        }
    }

    int porta() const { return porta_; }

private:
    QProcess processo_;
    int porta_ = 0;
};

} // namespace

class TesteTransporteHttps : public QObject {
    Q_OBJECT

private slots:
    void semCertificadoDeClienteHandshakeRecusado();
    void certificadoDeOutraAcRecusado();
    void timeoutViraErroTransitorio();
    void baseEPrefixoVemDaConfiguracao();
    void conexaoRecusadaViraErroTransitorio();
};

void TesteTransporteHttps::semCertificadoDeClienteHandshakeRecusado()
{
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, "servidor falso não subiu");
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
}

void TesteTransporteHttps::certificadoDeOutraAcRecusado()
{
    // Certificado autoassinado de outra raiz (cadeia_outra_raiz.pfx): o servidor só confia na AC de teste.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, "servidor falso não subiu");
    const ParCliente outra = parDoPfx(QStringLiteral("cadeia_outra_raiz.pfx"));
    QVERIFY2(!outra.certificado.isNull() && !outra.chave.isNull(), "não foi possível ler o par de outra AC");
    ConfiguracaoTls tls;
    tls.certificadoCliente = outra.certificado;
    tls.chaveCliente = outra.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
}

void TesteTransporteHttps::timeoutViraErroTransitorio()
{
    // O servidor segura a resposta de GET /dps/ABC (falha roteirizada, vale uma vez). O cliente desiste antes.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("timeout")});
    // (uma única tripla: METODO, CAMINHO, FALHA)
    QVERIFY2(servidor.porta() > 0, "servidor falso não subiu");
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 700ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Timeout);
}

void TesteTransporteHttps::baseEPrefixoVemDaConfiguracao()
{
    // O servidor responde 404 para rota desconhecida (ROTA_INEXISTENTE) e 404 com E2404 para a DPS sem nota. O corpo
    // diz qual rota o servidor recebeu: o prefixo da configuração tem de estar no caminho.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, "servidor falso não subiu");
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();

    TransporteHttps semPrefixo(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto r1 = semPrefixo.enviar(consultaDps());
    QVERIFY2(r1.has_value(), "sem prefixo: a requisição devia chegar");
    QCOMPARE(r1->status, 404);
    QVERIFY2(r1->corpo.contains("E2404"), "sem prefixo: o servidor devia responder pela rota /dps/");

    TransporteHttps comPrefixo(urlBase(servidor.porta(), QStringLiteral("/v1"), 5000ms), tls);
    auto r2 = comPrefixo.enviar(consultaDps());
    QVERIFY2(r2.has_value(), "com prefixo /v1: a requisição devia chegar");
    QCOMPARE(r2->status, 404);
    QVERIFY2(r2->corpo.contains("ROTA_INEXISTENTE"), "com prefixo: a rota /v1/dps/ não existe no servidor");
}

void TesteTransporteHttps::conexaoRecusadaViraErroTransitorio()
{
    // Sem servidor escutando na porta 1: conexão recusada é transitória (vale tentar de novo).
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(1, QString(), 3000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Conexao);
}

QTEST_GUILESS_MAIN(TesteTransporteHttps)
#include "tst_transporte_https.moc"
