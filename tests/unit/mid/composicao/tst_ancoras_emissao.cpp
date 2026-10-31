// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras e ambiente da emissão (NL-2.10b, emendas 4, 5 e 2, MÉDIA-1): fora de Homologação, o A1 usa as raízes ICP embutidas no
// bundle, e o servidor TLS usa as CAs do sistema do runtime. O ambiente vem do host, e não de quem chama a Mid.
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/mid/composicao/emissao_cli.h>
#include <nl/mid/composicao/politica_emissao.h>
#include <nl/mid/composicao/registro_cli.h>

#include <chrono>
#include <sstream>

#include <QFile>
#include <QIODevice>
#include <QList>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QTemporaryDir>
#include <QtTest>

namespace {

// Grava um certificado num PEM da pasta e devolve o caminho. Sem arquivo, o teste reprova pela asserção seguinte.
QString escreverPem(const QTemporaryDir& pasta, const QString& nome, const QSslCertificate& certificado)
{
    const QString caminho = pasta.filePath(nome);
    QFile arquivo(caminho);
    if (arquivo.open(QIODevice::WriteOnly)) {
        arquivo.write(certificado.toPem());
    }
    return caminho;
}

} // namespace

class TstAncorasEmissao : public QObject
{
    Q_OBJECT

private slots:
    // Fora de Homologação, o A1 usa o conjunto de produção do bundle, e caminhoAncoras é ignorado.
    void ancorasPadraoSaoAsRaizesIcp();
    // Sem [sefin] ancoras_tls, o servidor TLS usa as CAs do sistema; com o PEM, usa só o PEM.
    void ancorasTlsPadraoDoSistema();
    // NL-2.10b, MÉDIA-1: sem ambiente afirmado, o derivado do host manda. Host de Produção: CAs do sistema, mesmo com PEM.
    void pedidoSemAmbienteUsaODerivadoDoHost();
    // NL-2.10b, MÉDIA-1: ambiente afirmado diferente do host é recusado pela Mid, antes de qualquer leitura ou envio.
    void ambienteDivergenteDoHostRecusadoNaMid();
    // NL-2.10b, MÉDIA-1: raiz ICP com diretório vazio é erro de configuração (configuracao.raizes_diretorio_vazio), nunca leitura de /SHA256SUMS.
    void diretorioDeRaizesVazioEErroDeConfiguracao();
    // NL-2.10b, A5: em Homologação com [certificado] ancoras, a emissão carrega o PEM, e não as raízes ICP (guarda contra M7).
    void homologacaoUsaOPemDeAncoras();
};

void TstAncorasEmissao::ancorasPadraoSaoAsRaizesIcp()
{
    nl::mid::composicao::PedidoEmissao pedido;
    pedido.ambiente = nl::mid::composicao::Ambiente::Producao;
    pedido.diretorioRaizesIcp = QStringLiteral(NL_ICP_RAIZES_DIR);
    // Um caminho inexistente tem de ser ignorado: fora de Homologação, o PEM de [certificado] ancoras nem é lido.
    pedido.caminhoAncoras = QStringLiteral("/nao/existe/a1.pem");
    const auto ancoras = nl::mid::composicao::ancorasDoA1(pedido, std::chrono::system_clock::now());
    QVERIFY2(ancoras.has_value(), "fora de Homologação, o A1 tem de usar as raízes ICP embutidas");
    QVERIFY2(ancoras->quantidade() > 0, "o conjunto de produção não pode ser vazio");
}

void TstAncorasEmissao::ancorasTlsPadraoDoSistema()
{
    nl::mid::composicao::PedidoEmissao pedido;
    const QList<QSslCertificate> padrao = nl::mid::composicao::ancorasTlsDoPedido(pedido);
    QVERIFY2(!padrao.isEmpty(), "sem [sefin] ancoras_tls, o runtime tem de trazer as CAs do sistema");
    QVERIFY2(padrao == QSslConfiguration::systemCaCertificates(), "o padrão do servidor TLS tem de ser o repositório de CAs do sistema");

    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    pedido.caminhoAncorasTls = escreverPem(pasta, QStringLiteral("tls.pem"), padrao.first());
    const QList<QSslCertificate> doPem = nl::mid::composicao::ancorasTlsDoPedido(pedido);
    QCOMPARE(doPem.size(), qsizetype(1));
    QVERIFY(doPem.first() == padrao.first());
}

void TstAncorasEmissao::pedidoSemAmbienteUsaODerivadoDoHost()
{
    const QList<QSslCertificate> sistema = QSslConfiguration::systemCaCertificates();
    QVERIFY2(!sistema.isEmpty(), "o runtime de teste tem de trazer CAs do sistema");
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::PedidoEmissao pedido;  // nenhum ambiente afirmado
    pedido.urlSefin = QStringLiteral("https://sefin.nfse.gov.br/SefinNacional");  // host de Produção
    pedido.caminhoAncorasTls = escreverPem(pasta, QStringLiteral("tls.pem"), sistema.first());
    const QList<QSslCertificate> tls = nl::mid::composicao::ancorasTlsDoPedido(pedido);
    QVERIFY2(tls == sistema, "sem ambiente afirmado, o derivado do host (Produção) manda: CAs do sistema, sem o PEM");
}

void TstAncorasEmissao::ambienteDivergenteDoHostRecusadoNaMid()
{
    nl::mid::composicao::PedidoEmissao pedido;
    pedido.ambiente = nl::mid::composicao::Ambiente::Homologacao;  // afirma Homologação
    pedido.urlSefin = QStringLiteral("https://sefin.nfse.gov.br/SefinNacional");  // o host é de Produção
    pedido.dpsJson = QByteArrayLiteral("{}");  // a recusa tem de vir antes da DPS, das âncoras e do envio
    auto senha = nl::foundation::pkcs12::SenhaSegura::de(QByteArrayLiteral("x"));
    std::ostringstream destino;
    auto registro = nl::mid::composicao::criarLogDaCli(destino);
    const auto emitido = nl::mid::composicao::emitirDpsComAssinador(pedido, std::move(senha), *registro, nl::mid::composicao::FabricaAssinador{});
    QVERIFY2(!emitido.has_value(), "ambiente afirmado diferente do host tem de ser recusado antes de qualquer acesso");
    QCOMPARE(QString::fromStdString(emitido.error().codigo), QStringLiteral("emissao.ambiente_divergente"));
}

void TstAncorasEmissao::diretorioDeRaizesVazioEErroDeConfiguracao()
{
    nl::mid::composicao::PedidoEmissao pedido;
    pedido.ambiente = nl::mid::composicao::Ambiente::Producao;
    pedido.diretorioRaizesIcp = QString();  // sem diretório, a carga não pode ler "/SHA256SUMS"
    const auto ancoras = nl::mid::composicao::ancorasDoA1(pedido, std::chrono::system_clock::now());
    QVERIFY2(!ancoras.has_value(), "diretório de raízes vazio tem de ser recusado");
    QCOMPARE(QString::fromStdString(ancoras.error().codigo), QStringLiteral("configuracao.raizes_diretorio_vazio"));
}

void TstAncorasEmissao::homologacaoUsaOPemDeAncoras()
{
    const QList<QSslCertificate> sistema = QSslConfiguration::systemCaCertificates();
    QVERIFY2(!sistema.isEmpty(), "o runtime de teste tem de trazer CAs do sistema");
    QTemporaryDir pasta;
    QVERIFY(pasta.isValid());
    nl::mid::composicao::PedidoEmissao pedido;
    pedido.ambiente = nl::mid::composicao::Ambiente::Homologacao;
    pedido.caminhoAncoras = escreverPem(pasta, QStringLiteral("a1.pem"), sistema.first());
    const auto ancoras = nl::mid::composicao::ancorasDoA1(pedido, std::chrono::system_clock::now());
    QVERIFY2(ancoras.has_value(), "em Homologação, o PEM de [certificado] ancoras tem de carregar");
    QCOMPARE(ancoras->quantidade(), std::size_t(1));  // o conjunto de produção tem várias raízes; um PEM de uma tem de dar uma
}

QTEST_APPLESS_MAIN(TstAncorasEmissao)
#include "tst_ancoras_emissao.moc"
