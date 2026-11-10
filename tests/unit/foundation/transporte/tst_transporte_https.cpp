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
#include <QJsonArray>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtTest>

#include <chrono>
#include <cerrno>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

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
    // extras: opções do lançador (--tls-maximo, --fechamento-adiado, ...), passadas como estão (NL-3.0j).
    explicit ServidorDeTeste(const QStringList& falha = {}, const QStringList& extras = {}) {
        QStringList argumentos{QStringLiteral("-I"), QStringLiteral(NL_LANCADOR), pastaFixtures()};
        if (!falha.isEmpty()) {
            argumentos << QStringLiteral("--falha") << falha;
        }
        argumentos << extras;
        processo_.start(QStringLiteral(NL_PYTHON3), argumentos);
        if (processo_.waitForReadyRead(15000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
        if (porta_ == 0) {
            // Sem porta, o traceback do lançador é a única pista: guarda-o para a mensagem do QVERIFY2.
            processo_.kill();
            processo_.waitForFinished(2000);
            diagnostico_ = QStringLiteral("servidor falso não subiu; stderr do lançador: ")
                           + QString::fromUtf8(processo_.readAllStandardError());
        }
    }

    const QString& diagnostico() const { return diagnostico_; }

    // Fecha a entrada, espera o lançador sair e devolve as linhas que ele escreveu depois da porta (NL-3.0j, M2).
    QStringList encerrar() {
        processo_.closeWriteChannel();
        if (!processo_.waitForFinished(5000)) {
            processo_.kill();
            processo_.waitForFinished(2000);
        }
        QStringList linhas;
        const QString saida = QString::fromUtf8(processo_.readAllStandardOutput());
        for (const QString& linha : saida.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            linhas << linha.trimmed();
        }
        return linhas;
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
    QString diagnostico_;
};

} // namespace

// Servidor que recusa HTTP/2 com HTTP_1_1_REQUIRED (NL-2.10e, E5, DA-2.80, tools/servidor_falso/servidor_h2.py). Escreve "ALPN <valor>"
// por conexão e "HTTP200" quando responde em HTTP/1.1. Só a biblioteca padrão do Python.
class ServidorH2Teste {
public:
    explicit ServidorH2Teste(const QStringList& extras = {}) {
        QStringList argumentos{QStringLiteral(NL_SERVIDOR_H2), pastaFixtures()};
        argumentos << extras;
        processo_.start(QStringLiteral(NL_PYTHON3), argumentos);
        if (processo_.waitForReadyRead(15000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
        if (porta_ == 0) {
            processo_.kill();
            processo_.waitForFinished(2000);
            diagnostico_ = QStringLiteral("servidor h2 falso não subiu; stderr: ") + QString::fromUtf8(processo_.readAllStandardError());
        }
    }
    ~ServidorH2Teste() {
        processo_.kill();
        processo_.waitForFinished(2000);
    }
    int porta() const { return porta_; }
    const QString& diagnostico() const { return diagnostico_; }

    // Linhas que o servidor escreveu até aparecer a marca, ou até o tempo acabar.
    QStringList linhasAte(const QString& marca, int milissegundos) {
        QStringList todas;
        QByteArray resto;
        QElapsedTimer relogio;
        relogio.start();
        while (relogio.elapsed() < milissegundos && !todas.contains(marca)) {
            // Lê o que já está no buffer do QProcess antes de esperar: waitForReadyRead só acorda com dado novo.
            resto += processo_.readAllStandardOutput();
            processo_.waitForReadyRead(100);
            qsizetype fim = resto.indexOf('\n');
            while (fim >= 0) {
                todas << QString::fromUtf8(resto.left(fim)).trimmed();
                resto.remove(0, fim + 1);
                fim = resto.indexOf('\n');
            }
        }
        return todas;
    }

private:
    QProcess processo_;
    int porta_ = 0;
    QString diagnostico_;
};

// Medição por execução do slot (NL-3.0j, CTO passo 2). Uma linha `medicao` em qInfo: o -V do ctest a leva ao log, e o arnês
// de ensaio (tools/conteiner/ensaio_no_conteiner.sh) conta essas linhas. A repetição não mora no teste: cada repetição é um
// processo novo, criado pelo ctest --repeat until-fail.
namespace {

const char* const kMensagemRecusaDepoisDoHandshake = "servidor recusou depois do handshake, provável certificado do cliente";

void gravarMedicao(const QString& linha)
{
    qInfo().noquote() << "medicao" << linha;
}

// Uma linha por execução: o que o slot precisa para explicar um vermelho sozinho.
void registrarMedicao(const QString& slot, const ErroTransporte& erro, int conexoes,
                      const QString& extra = QString())
{
    gravarMedicao(QStringLiteral("%1;tipo=%2;causa=%3;codigoQt=%4;handshake=%5;versaoTls=%6;bytesEscritos=%7;conexoes=%8;mensagem=%9%10")
        .arg(slot)
        .arg(static_cast<int>(erro.tipo))
        .arg(static_cast<int>(erro.causa))
        .arg(erro.codigoQt)
        .arg(erro.handshake ? 1 : 0)
        .arg(QString::fromStdString(erro.versaoTls))
        .arg(erro.bytesEscritos)
        .arg(conexoes)
        .arg(erro.mensagem)
        .arg(extra.isEmpty() ? QString() : QStringLiteral(";") + extra));
}

// Linhas CONEXOES do lançador. Ausência da linha devolve -1: é falha de medição, nunca zero (criterio, NL-3.0j).
int conexoesDoServidor(const QStringList& linhas)
{
    for (const QString& linha : linhas) {
        if (linha.startsWith(QStringLiteral("CONEXOES "))) {
            return linha.mid(9).toInt();
        }
    }
    return -1;
}

// Linha DRENAGEM_TETO do lançador: o cliente não fechou dentro do teto do fechamento adiado (falha do slot).
bool drenagemEstourou(const QStringList& linhas)
{
    return linhas.contains(QStringLiteral("DRENAGEM_TETO"));
}

} // namespace

class TesteTransporteHttps : public QObject {
    Q_OBJECT

private slots:
    // initTestCase e cleanupTestCase existem de propósito: o QTest soma os dois como passes, e o CTest espera
    // "Totals: 3 passed" (um slot + dois). Sem eles, a contagem do regex não bate (medido pelo revisor, fix(test)).
    void initTestCase();
    void cleanupTestCase();
    void semCertificadoDeClienteHandshakeRecusado();
    void certificadoDeOutraAcRecusado();
    void timeoutViraErroTransitorio();
    void baseEPrefixoVemDaConfiguracao();
    void conexaoRecusadaViraErroTransitorio();
    void corpo2xxTruncadoViraTransitorio();
    void rejeicao400ComCorpoContinuaRespostaHttp();
    // NL-2.10e (E5, DA-2.79): o servidor só fala h2 e recusa com HTTP_1_1_REQUIRED; o transporte tem de negociar HTTP/1.1 e receber 200.
    void h2RecusadoRepetidoEmHttp11();
    // NL-2.10e (R-B3, piso TLS 1.2): um servidor que só fala TLS 1.1 não pode ser aceito: o handshake tem de falhar com causa Tls.
    void servidorSoTls11RecusadoPeloPiso();
    void pisoTls12NaConfiguracaoDeProducao();
    void respostaTrazOContentType();
    void cabecalhoAcceptEncodingIdentityEnviado();
    void contentEncodingNaoIdentityViraDefinitivoProtocolo();
    void servidorDeOutraRaizRecusado();
    void headSemCorpoNaoViraErroDeRede();
    void corpoAcimaDoTetoRecusado();
    void certificadoSemChaveRecusadoComoConfiguracao();
    void chaveSemCertificadoRecusadoComoConfiguracao();
    void redirecionamentoNaoSeguidoViraErro();
    void tempoLimiteAcimaDoTetoRecusado();
    void prefixoInvalidoRecusadoComoConfiguracao();
    void mensagemDeErroNaoTrazURLNemHost();
    // NL-3.0j (ordem do CISO, seção 4): sonda do RST, recusa em TLS 1.3 com fechamento adiado, corte com RST depois do
    // handshake, e o POST lido antes do RST (M1: quantas vezes o Qt envia o mesmo POST).
    void servidorFalsoCortaComRstNaoFin();
    void recusaSemCertificadoTls13FechamentoAdiado();
    void recusaOutraAcTls13FechamentoAdiado();
    void corteSemAlertaTls13Transitorio();
    void postLidoComCorteRstNaoReenviaQt();
};

void TesteTransporteHttps::initTestCase()
{
    // Se o plugin TLS do Qt não carregou no contêiner, os testes de handshake falhariam por motivo que não é o transporte.
    QVERIFY2(QSslSocket::supportsSsl(), qPrintable(QSslSocket::sslLibraryBuildVersionString()));
    QVERIFY2(!pastaFixtures().isEmpty(), "NL_CERTIFICADOS_DIR não definido: a fixture de certificados não rodou");
}

void TesteTransporteHttps::cleanupTestCase()
{
}

void TesteTransporteHttps::servidorSoTls11RecusadoPeloPiso()
{
    ServidorH2Teste servidor({QStringLiteral("--tls11")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 8000ms), tls);
    const auto resposta = transporte.enviar(consultaDps());
    QVERIFY2(!resposta.has_value(), "TLS abaixo de 1.2 não pode ser aceito (piso do canon, R-B3)");
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
}

// NL-2.10e (R-B3, T3): o piso é afirmado na configuração que a conexão usa, não só pelo handshake. O handshake com servidor
// só-1.1 falha por política do sistema (MinProtocol=TLSv1.2 do OpenSSL) mesmo com AnyProtocol, então esse teste não mata T3 sozinho.
void TesteTransporteHttps::pisoTls12NaConfiguracaoDeProducao()
{
    const ConfiguracaoTls tls;
    const QSslConfiguration cfg = configuracaoSslDoTransporte(tls);
    QVERIFY2(cfg.protocol() == QSsl::TlsV1_2OrLater, "a conexão tem de pedir TLS 1.2 ou mais novo (piso do canon)");
}

void TesteTransporteHttps::h2RecusadoRepetidoEmHttp11()
{
    ServidorH2Teste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 8000ms), tls);
    const auto resposta = transporte.enviar(consultaDps());
    const QStringList linhas = servidor.linhasAte(QStringLiteral("HTTP200"), 8000);
    qInfo().noquote() << "servidor h2 falso:" << linhas.join(QStringLiteral(" | "));
    QVERIFY2(linhas.contains(QStringLiteral("ALPN http/1.1")),
             "o transporte tem de negociar HTTP/1.1 no caminho autenticado (E5, DA-2.79); negociou outra coisa");
    QVERIFY2(resposta.has_value(), "com HTTP/1.1 forçado, a requisição tem de receber resposta");
    QCOMPARE(resposta->status, 200);
}

void TesteTransporteHttps::semCertificadoDeClienteHandshakeRecusado()
{
    // NL-3.0j (CTO, passo 3; CISO, seção 4): TLS 1.2 com fechamento adiado. O TLS 1.2 recusa antes do Finished do cliente, então
    // nada chega à aplicação: versaoTls "nenhuma", zero bytes e uma conexão só.
    ServidorDeTeste servidor({}, {QStringLiteral("--tls-maximo"), QStringLiteral("1.2"), QStringLiteral("--fechamento-adiado")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    const QStringList linhas = servidor.encerrar();
    registrarMedicao(QStringLiteral("semCertificadoDeClienteHandshakeRecusado"), resposta.error(),
                     conexoesDoServidor(linhas), QStringLiteral("drenagemEstourou=%1").arg(drenagemEstourou(linhas) ? 1 : 0));
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
    QCOMPARE(QString::fromStdString(resposta.error().versaoTls), QStringLiteral("nenhuma"));
    QCOMPARE(resposta.error().bytesEscritos, static_cast<qint64>(0));
    QCOMPARE(conexoesDoServidor(linhas), 1);
    QVERIFY2(!drenagemEstourou(linhas), "o cliente não fechou dentro do teto do fechamento adiado");
}

void TesteTransporteHttps::certificadoDeOutraAcRecusado()
{
    // Certificado autoassinado de outra raiz (cadeia_outra_raiz.pfx): o servidor só confia na AC de teste.
    // NL-3.0j (CTO, passo 3; CISO, seção 4): TLS 1.2 com fechamento adiado, como o slot anterior.
    ServidorDeTeste servidor({}, {QStringLiteral("--tls-maximo"), QStringLiteral("1.2"), QStringLiteral("--fechamento-adiado")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente outra = parDoPfx(QStringLiteral("cadeia_outra_raiz.pfx"));
    QVERIFY2(!outra.certificado.isNull() && !outra.chave.isNull(), "não foi possível ler o par de outra AC");
    ConfiguracaoTls tls;
    tls.certificadoCliente = outra.certificado;
    tls.chaveCliente = outra.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    const QStringList linhas = servidor.encerrar();
    registrarMedicao(QStringLiteral("certificadoDeOutraAcRecusado"), resposta.error(), conexoesDoServidor(linhas),
                     QStringLiteral("drenagemEstourou=%1").arg(drenagemEstourou(linhas) ? 1 : 0));
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
    QCOMPARE(QString::fromStdString(resposta.error().versaoTls), QStringLiteral("nenhuma"));
    QCOMPARE(resposta.error().bytesEscritos, static_cast<qint64>(0));
    QCOMPARE(conexoesDoServidor(linhas), 1);
    QVERIFY2(!drenagemEstourou(linhas), "o cliente não fechou dentro do teto do fechamento adiado");
}

// Sonda do RST (NL-3.0j, MR1). Conexão crua recebe ECONNRESET, não EOF: com FIN, recv devolve 0. Sem o SO_LINGER do servidor,
// o slot tem de ficar vermelho. O timeout de recepção impede que a sonda trave se o servidor não responder.
void TesteTransporteHttps::servidorFalsoCortaComRstNaoFin()
{
    ServidorDeTeste servidor({}, {QStringLiteral("--corte-rst-antes-tls")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const int descritor = ::socket(AF_INET, SOCK_STREAM, 0);
    QVERIFY2(descritor >= 0, "socket cru não abriu");
    timeval tempo{5, 0};
    ::setsockopt(descritor, SOL_SOCKET, SO_RCVTIMEO, &tempo, sizeof tempo);
    sockaddr_in destino{};
    destino.sin_family = AF_INET;
    destino.sin_port = htons(static_cast<uint16_t>(servidor.porta()));
    ::inet_pton(AF_INET, "127.0.0.1", &destino.sin_addr);
    const bool conectou = ::connect(descritor, reinterpret_cast<sockaddr*>(&destino), sizeof destino) == 0;
    QVERIFY2(conectou, "conexão crua ao servidor falso não completou");
    char byte = 0;
    const ssize_t lidos = ::recv(descritor, &byte, 1, 0);
    const int erro = errno;
    ::close(descritor);
    const QStringList linhas = servidor.encerrar();
    gravarMedicao(QStringLiteral("servidorFalsoCortaComRstNaoFin;lidos=%1;errno=%2;conexoes=%3")
                      .arg(static_cast<qlonglong>(lidos)).arg(erro).arg(conexoesDoServidor(linhas)));
    QVERIFY2(lidos == -1 && erro == ECONNRESET, "o servidor tem de cortar com RST (ECONNRESET), não com FIN (EOF)");
    QCOMPARE(conexoesDoServidor(linhas), 1);
}

namespace {

// Recusa em TLS 1.3 com fechamento adiado (NL-3.0j, ordem do CISO, seção 4). O cliente conclui o handshake antes de saber da
// recusa (Go 32202; RFC 8446 §2); o servidor manda o alerta e lê até o cliente fechar. O desfecho esperado é o da DA-2.35:
// Definitivo/Tls, com a mensagem fixa e versaoTls tls1.3, numa só conexão, sem drenagem estourada.
void verificarRecusaTls13ComFechamentoAdiado(const QString& slot, const ConfiguracaoTls& tls)
{
    ServidorDeTeste servidor({}, {QStringLiteral("--fechamento-adiado")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    const QStringList linhas = servidor.encerrar();
    registrarMedicao(slot, resposta.error(), conexoesDoServidor(linhas),
                     QStringLiteral("drenagemEstourou=%1").arg(drenagemEstourou(linhas) ? 1 : 0));
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
    QCOMPARE(QString::fromStdString(resposta.error().versaoTls), QStringLiteral("tls1.3"));
    QCOMPARE(resposta.error().mensagem, QString::fromUtf8(kMensagemRecusaDepoisDoHandshake));
    QCOMPARE(conexoesDoServidor(linhas), 1);
    QVERIFY2(!drenagemEstourou(linhas), "o cliente não fechou dentro do teto do fechamento adiado");
}

} // namespace

void TesteTransporteHttps::recusaSemCertificadoTls13FechamentoAdiado()
{
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    verificarRecusaTls13ComFechamentoAdiado(QStringLiteral("recusaSemCertificadoTls13FechamentoAdiado"), tls);
}

void TesteTransporteHttps::recusaOutraAcTls13FechamentoAdiado()
{
    const ParCliente outra = parDoPfx(QStringLiteral("cadeia_outra_raiz.pfx"));
    QVERIFY2(!outra.certificado.isNull() && !outra.chave.isNull(), "não foi possível ler o par de outra AC");
    ConfiguracaoTls tls;
    tls.certificadoCliente = outra.certificado;
    tls.chaveCliente = outra.chave;
    tls.autoridades = autoridadeDeTeste();
    verificarRecusaTls13ComFechamentoAdiado(QStringLiteral("recusaOutraAcTls13FechamentoAdiado"), tls);
}

// Corte com RST depois do handshake, com certificado válido (NL-3.0j, ordem do CISO). Vale para toda conexão do slot, porque o
// Qt pode reenviar (CISO, seção 0). A asserção pede pelo menos uma conexão e Transitorio/Conexao, sem fixar o número.
void TesteTransporteHttps::corteSemAlertaTls13Transitorio()
{
    ServidorDeTeste servidor({}, {QStringLiteral("--corte-rst-apos-handshake")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    const QStringList linhas = servidor.encerrar();
    registrarMedicao(QStringLiteral("corteSemAlertaTls13Transitorio"), resposta.error(), conexoesDoServidor(linhas));
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Conexao);
    QCOMPARE(QString::fromStdString(resposta.error().versaoTls), QStringLiteral("tls1.3"));
    QVERIFY2(conexoesDoServidor(linhas) >= 1, "o servidor tem de ter recebido ao menos uma conexão (CONEXOES ausente é falha)");
}

// M1 (NL-3.0j, primeiro lote): o servidor lê o POST inteiro, registra e corta com RST antes de responder, uma vez. Quantas vezes o
// Qt envia o mesmo POST é o que se mede: as asserções valem para qualquer contagem (CISO, seção 4). Com mais de 1 POST por
// repetição, a fatia para e volta ao CISO e ao CTO.
void TesteTransporteHttps::postLidoComCorteRstNaoReenviaQt()
{
    ServidorDeTeste servidor({QStringLiteral("POST"), QStringLiteral("/nfse"), QStringLiteral("corte_rst_apos_leitura")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    const RequisicaoHttp envio{QByteArrayLiteral("POST"), QStringLiteral("/nfse"), QByteArrayLiteral("nao-e-envelope"),
                               QByteArrayLiteral("application/xml")};
    auto resposta = transporte.enviar(envio);
    QVERIFY(!resposta.has_value());
    // Conta os POST que o servidor leu: a conexão de /_teste/posts é uma conexão a mais, e sai do total de CONEXOES.
    TransporteHttps registro(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto posts = registro.enviar(RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/_teste/posts"), {}, {}});
    QVERIFY2(posts.has_value(), "o registro de POST tem de responder");
    const int quantos = QJsonDocument::fromJson(posts->corpo).array().size();
    const QStringList linhas = servidor.encerrar();
    const int conexoesDoEnvio = conexoesDoServidor(linhas) - 1;
    registrarMedicao(QStringLiteral("postLidoComCorteRstNaoReenviaQt"), resposta.error(), conexoesDoEnvio,
                     QStringLiteral("posts=%1").arg(quantos));
    QVERIFY2(quantos >= 1, "o POST tem de chegar ao servidor: é nele que o corte acontece");
    QVERIFY2(conexoesDoEnvio >= 1, "CONEXOES ausente ou zero é falha de medição");
}

void TesteTransporteHttps::timeoutViraErroTransitorio()
{
    // O servidor segura a resposta de GET /dps/ABC (falha roteirizada, vale uma vez). O cliente desiste antes.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("timeout")});
    // (uma única tripla: METODO, CAMINHO, FALHA)
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 700ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY(!resposta.has_value());
    qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Timeout);
}

void TesteTransporteHttps::baseEPrefixoVemDaConfiguracao()
{
    // O servidor responde 404 para rota desconhecida (ROTA_INEXISTENTE) e 404 com E2404 para a DPS sem nota. O corpo
    // diz qual rota o servidor recebeu: o prefixo da configuração tem de estar no caminho.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
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
    qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Conexao);
}

void TesteTransporteHttps::rejeicao400ComCorpoContinuaRespostaHttp()
{
    // POST /nfse com corpo que não é envelope: o servidor responde 400 ENVELOPE_INVALIDO. O Qt reporta 400 com um
    // NetworkError que o transporte não mapeia (ProtocolInvalidOperationError). Status recebido vale como resposta,
    // qualquer que seja o NetworkError: quem decide o que fazer com o 400 é o chamador.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    const RequisicaoHttp envioRuim{QByteArrayLiteral("POST"), QStringLiteral("/nfse"),
                                   QByteArrayLiteral("nao-e-envelope"), QByteArrayLiteral("application/xml")};
    auto resposta = transporte.enviar(envioRuim);
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY2(resposta.has_value(), "400 do servidor devia voltar como RespostaHttp, não como erro");
    QCOMPARE(resposta->status, 400);
    QVERIFY2(resposta->corpo.contains("ENVELOPE_INVALIDO"), "o corpo do 400 devia chegar intacto");
}

void TesteTransporteHttps::respostaTrazOContentType()
{
    // O servidor falso responde o 400 em JSON (Content-Type application/json; charset=utf-8). O transporte tem de devolver
    // esse cabeçalho em tipoConteudo, para o observador de respostas (NL-2.10c) gravar o tipo que a Sefin mandou.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    const RequisicaoHttp envioRuim{QByteArrayLiteral("POST"), QStringLiteral("/nfse"),
                                   QByteArrayLiteral("nao-e-envelope"), QByteArrayLiteral("application/xml")};
    auto resposta = transporte.enviar(envioRuim);
    QVERIFY2(resposta.has_value(), "400 do servidor devia voltar como RespostaHttp");
    QVERIFY2(resposta->tipoConteudo.startsWith("application/json"),
             qPrintable(QStringLiteral("Content-Type da resposta devia vir em tipoConteudo; recebido: '%1'")
                            .arg(QString::fromLatin1(resposta->tipoConteudo))));
}

void TesteTransporteHttps::mensagemDeErroNaoTrazURLNemHost()
{
    // Host que não resolve (domínio .invalid, RFC 6761). O Qt põe o host no errorString. A mensagem do erro vai para
    // log e tela, e não pode levar a URL da requisição (chave de acesso, NSU), nem o host, nem o caminho.
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    const UrlBase base{QUrl(QStringLiteral("https://nao-resolve.invalid:443")), QString(), 3000ms};
    TransporteHttps transporte(base, tls);
    const RequisicaoHttp consulta{QByteArrayLiteral("GET"), QStringLiteral("/dps/SEGREDO-NSU-123"), {}, {}};
    auto resposta = transporte.enviar(consulta);
    QVERIFY(!resposta.has_value());
    const QString mensagem = resposta.error().mensagem;
    qInfo().noquote() << "mensagem:" << mensagem;
    QVERIFY2(!mensagem.contains(QStringLiteral("nao-resolve.invalid")), "a mensagem não pode trazer o host");
    QVERIFY2(!mensagem.contains(QStringLiteral("SEGREDO")), "a mensagem não pode trazer o caminho da requisição");
    QVERIFY2(!mensagem.contains(QStringLiteral("https://")), "a mensagem não pode trazer a URL");
}

void TesteTransporteHttps::corpo2xxTruncadoViraTransitorio()
{
    // O servidor responde 200 com Content-Length e fecha a conexão no meio do corpo (falha roteirizada corpo_truncado).
    // Status com corpo incompleto não é resposta: o transporte devolve erro transitório de conexão, e o chamador não
    // recebe um corpo parcial como se fosse completo.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("corpo_truncado")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (resposta.has_value()) {
        qInfo().noquote() << "resposta com status" << resposta->status << "e" << resposta->corpo.size() << "bytes";
    } else {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY2(!resposta.has_value(), "corpo truncado não pode voltar como RespostaHttp");
    QCOMPARE(resposta.error().tipo, TipoErro::Transitorio);
    QCOMPARE(resposta.error().causa, CausaErro::Conexao);
}

void TesteTransporteHttps::cabecalhoAcceptEncodingIdentityEnviado()
{
    // Falha roteirizada eco_accept_encoding: o servidor devolve, como corpo, o Accept-Encoding que recebeu. A conferência
    // é do lado do servidor, então o valor é o que o transporte mandou de fato na requisição.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("eco_accept_encoding")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    QVERIFY2(resposta.has_value(), "o eco devia responder 200");
    QCOMPARE(resposta->status, 200);
    QCOMPARE(resposta->corpo, QByteArrayLiteral("identity"));
}

void TesteTransporteHttps::contentEncodingNaoIdentityViraDefinitivoProtocolo()
{
    // Falha roteirizada content_encoding_gzip: 200 com Content-Encoding gzip. O transporte pede identity, então qualquer
    // Content-Encoding diferente de identity na resposta é protocolo inesperado: erro definitivo, sem corpo para o chamador.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("content_encoding_gzip")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (resposta.has_value()) {
        qInfo().noquote() << "resposta com status" << resposta->status << "e" << resposta->corpo.size() << "bytes";
    }
    QVERIFY2(!resposta.has_value(), "Content-Encoding diferente de identity não pode voltar como RespostaHttp");
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Protocolo);
}

void TesteTransporteHttps::servidorDeOutraRaizRecusado()
{
    // Âncora de outra raiz (cadeia_outra_raiz.pfx): o servidor de teste apresenta certificado da AC de teste, que não
    // está na âncora. A verificação do certificado do SERVIDOR tem de recusar. Sem este teste, VerifyNone passa em tudo.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    const ParCliente outra = parDoPfx(QStringLiteral("cadeia_outra_raiz.pfx"));
    QVERIFY2(!outra.certificado.isNull(), "não foi possível ler a outra raiz");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = QList<QSslCertificate>{outra.certificado};
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY2(!resposta.has_value(), "servidor com cadeia fora da âncora não pode ser aceito");
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Tls);
}

void TesteTransporteHttps::headSemCorpoNaoViraErroDeRede()
{
    // HEAD devolve status e cabeçalhos, sem corpo. Não é erro de rede: o Content-Length da resposta não tem corpo a
    // conferir, e o parser do Qt já trata a conexão que cai antes do fim do corpo.
    ServidorDeTeste servidor;
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(RequisicaoHttp{QByteArrayLiteral("HEAD"), QStringLiteral("/dps/ABC"), {}, {}});
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY2(resposta.has_value(), "HEAD devia voltar como RespostaHttp, com corpo vazio");
    QCOMPARE(resposta->status, 404);
    QVERIFY(resposta->corpo.isEmpty());
}

void TesteTransporteHttps::corpoAcimaDoTetoRecusado()
{
    // Caso 1: Content-Length acima do teto. corpo_truncado anuncia 280 bytes e o teto é de 100: recusado antes da leitura.
    {
        ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("corpo_truncado")});
        QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
        const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
        QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
        ConfiguracaoTls tls;
        tls.certificadoCliente = cliente.certificado;
        tls.chaveCliente = cliente.chave;
        tls.autoridades = autoridadeDeTeste();
        TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
        const RequisicaoHttp pedido{QByteArrayLiteral("GET"), QStringLiteral("/dps/ABC"), {}, {}, 100};
        auto resposta = transporte.enviar(pedido);
        if (!resposta.has_value()) {
            qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
        }
        QVERIFY2(!resposta.has_value(), "Content-Length acima do teto não pode voltar como resposta");
        QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
        QCOMPARE(resposta.error().causa, CausaErro::Protocolo);
    }
    // Caso 2: sem Content-Length. O servidor manda 4096 bytes e fecha; com teto de 1024, o transporte aborta na leitura.
    {
        ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("corpo_sem_tamanho")});
        QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
        const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
        QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
        ConfiguracaoTls tls;
        tls.certificadoCliente = cliente.certificado;
        tls.chaveCliente = cliente.chave;
        tls.autoridades = autoridadeDeTeste();
        TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
        const RequisicaoHttp pedido{QByteArrayLiteral("GET"), QStringLiteral("/dps/ABC"), {}, {}, 1024};
        auto resposta = transporte.enviar(pedido);
        if (!resposta.has_value()) {
            qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
        }
        QVERIFY2(!resposta.has_value(), "corpo acima do teto sem Content-Length não pode voltar como resposta");
        QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
        QCOMPARE(resposta.error().causa, CausaErro::Protocolo);
    }
}

void TesteTransporteHttps::certificadoSemChaveRecusadoComoConfiguracao()
{
    // Certificado sem chave: erro de configuração antes de enviar. A porta 1 não tem servidor: se o transporte tentasse
    // enviar, daria erro de conexão, e o teste acusa a diferença.
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull(), "não foi possível ler o certificado de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(1, QString(), 3000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Configuracao);
}

void TesteTransporteHttps::chaveSemCertificadoRecusadoComoConfiguracao()
{
    // Chave sem certificado: o mesmo erro de configuração, antes de enviar.
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.chave.isNull(), "não foi possível ler a chave de cliente");
    ConfiguracaoTls tls;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(1, QString(), 3000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Configuracao);
}

void TesteTransporteHttps::redirecionamentoNaoSeguidoViraErro()
{
    // A falha redireciona: GET /dps/ABC responde 302 para /dps/SEGUIDO. O transporte não segue e trata 3xx como erro
    // definitivo de protocolo. Se seguisse, a segunda chamada daria 404, e isso viraria resposta: o teste acusa.
    ServidorDeTeste servidor({QStringLiteral("GET"), QStringLiteral("/dps/ABC"), QStringLiteral("redireciona")});
    QVERIFY2(servidor.porta() > 0, qPrintable(servidor.diagnostico()));
    const ParCliente cliente = parDoPfx(QStringLiteral("cnpj_ok.pfx"));
    QVERIFY2(!cliente.certificado.isNull() && !cliente.chave.isNull(), "não foi possível ler o par de cliente");
    ConfiguracaoTls tls;
    tls.certificadoCliente = cliente.certificado;
    tls.chaveCliente = cliente.chave;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(servidor.porta(), QString(), 5000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    } else {
        qInfo().noquote() << "resposta com status" << resposta->status << "(redirecionamento não devia virar resposta)";
    }
    QVERIFY2(!resposta.has_value(), "3xx não pode voltar como resposta: o redirecionamento não é seguido");
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Protocolo);
}

void TesteTransporteHttps::tempoLimiteAcimaDoTetoRecusado()
{
    // Tempo limite de 3e9 ms, acima de INT_MAX e do teto de 10 minutos: configuração inválida, sem estouro de int.
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    const UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:1")), QString(), std::chrono::milliseconds{3000000000LL}};
    TransporteHttps transporte(base, tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Configuracao);
}

void TesteTransporteHttps::prefixoInvalidoRecusadoComoConfiguracao()
{
    // Prefixo "v1", sem barra inicial: a configuração é inválida, e o caminho sairia colado (v1/dps/ABC).
    ConfiguracaoTls tls;
    tls.autoridades = autoridadeDeTeste();
    TransporteHttps transporte(urlBase(1, QStringLiteral("v1"), 3000ms), tls);
    auto resposta = transporte.enviar(consultaDps());
    if (!resposta.has_value()) {
        qInfo().noquote() << "mensagem:" << resposta.error().mensagem;
    }
    QVERIFY(!resposta.has_value());
    QCOMPARE(resposta.error().tipo, TipoErro::Definitivo);
    QCOMPARE(resposta.error().causa, CausaErro::Configuracao);
}

QTEST_GUILESS_MAIN(TesteTransporteHttps)
#include "tst_transporte_https.moc"
