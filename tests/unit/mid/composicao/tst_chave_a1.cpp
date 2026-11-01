// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste da chave do A1 na configuração TLS da emissão (NL-2.10e, E3, H1). Chama a função de produção (configuracaoTlsDaCredencial) com
// um .pfx de FIXTURE de teste, nunca o do líder. Não há senha real nem certificado real aqui.
#include <nl/mid/composicao/emissao_cli.h>

#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/transporte/transporte_https.h>

#include <optional>

#include <QFile>
#include <QProcess>
#include <QtTest>

namespace fp = nl::foundation::pkcs12;
namespace ft = nl::foundation::transporte;
namespace mc = nl::mid::composicao;

namespace {

QString pastaFixtures()
{
    return qEnvironmentVariable("NL_CERTIFICADOS_DIR");
}

QByteArray lerFixture(const QString& nome)
{
    QFile arquivo(pastaFixtures() + QLatin1Char('/') + nome);
    return arquivo.open(QIODevice::ReadOnly) ? arquivo.readAll() : QByteArray();
}

// Credencial da fixture cnpj_ok.pfx, validada contra a AC de teste. Nenhum .pfx real.
std::optional<fp::CredencialX509> credencialDeTeste()
{
    const auto ancoras = fp::Ancoras::dePem(lerFixture(QStringLiteral("ac.pem")));
    if (!ancoras) {
        return std::nullopt;
    }
    fp::CarregadorPkcs12 carregador;
    auto credencial = carregador.carregarConsumindo(lerFixture(QStringLiteral("cnpj_ok.pfx")),
                                                    fp::SenhaSegura::de(lerFixture(QStringLiteral("senha_teste.txt")).trimmed()), *ancoras);
    if (!credencial) {
        return std::nullopt;
    }
    return std::move(*credencial);
}

// Servidor mTLS falso em processo próprio (lancador), que exige certificado de cliente. Lê a porta da primeira linha.
class ServidorMtlsTeste {
public:
    ServidorMtlsTeste()
    {
        processo_.start(QStringLiteral(NL_PYTHON3), {QStringLiteral("-I"), QStringLiteral(NL_LANCADOR), pastaFixtures()});
        if (processo_.waitForReadyRead(15000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
    }
    ~ServidorMtlsTeste()
    {
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

class TstChaveA1 : public QObject
{
    Q_OBJECT

private slots:
    // H1: a chave do A1, importada como DER na configuração de produção, não sai nula.
    void chaveDaFixtureNaoSaiNula();
    // Com a configuração de produção, o handshake mTLS com o servidor falso conclui (a causa do erro não é Tls).
    void handshakeMtlsConcluiComAChaveDeProducao();
};

void TstChaveA1::chaveDaFixtureNaoSaiNula()
{
    const auto credencial = credencialDeTeste();
    QVERIFY2(credencial.has_value(), "a fixture cnpj_ok.pfx tem de carregar com a AC de teste");
    const ft::ConfiguracaoTls tls = mc::configuracaoTlsDaCredencial(*credencial, {});
    QVERIFY2(!tls.certificadoCliente.isNull(), "o certificado do cliente tem de sair na configuração");
    QVERIFY2(!tls.chaveCliente.isNull(), "H1: a chave do A1 importada como DER pela produção saiu nula");
}

void TstChaveA1::handshakeMtlsConcluiComAChaveDeProducao()
{
    const auto credencial = credencialDeTeste();
    QVERIFY2(credencial.has_value(), "a fixture cnpj_ok.pfx tem de carregar com a AC de teste");
    ServidorMtlsTeste servidor;
    QVERIFY2(servidor.porta() > 0, "o servidor mTLS falso tem de subir");
    ft::ConfiguracaoTls tls = mc::configuracaoTlsDaCredencial(*credencial, QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem));
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(servidor.porta())), QString(), std::chrono::milliseconds{8000}};
    ft::TransporteHttps transporte(base, tls);
    const auto resposta = transporte.enviar(ft::RequisicaoHttp{QByteArrayLiteral("GET"), QStringLiteral("/dps/ABC"), {}, {}});
    if (!resposta) {
        QVERIFY2(resposta.error().causa != ft::CausaErro::Tls,
                 qPrintable(QStringLiteral("o handshake mTLS não concluiu: %1").arg(resposta.error().mensagem)));
    }
}

// QTEST_MAIN e não APPLESS: o transporte espera a resposta com QEventLoop, que exige QCoreApplication (sem ela, o teste trava).
QTEST_MAIN(TstChaveA1)
#include "tst_chave_a1.moc"
