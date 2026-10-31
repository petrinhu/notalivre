// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras da emissão (NL-2.10b, emendas 4 e 5): fora de Homologação, o A1 usa as raízes ICP embutidas no bundle, e o servidor
// TLS usa as CAs do sistema do runtime quando a configuração não traz o PEM.
#include <nl/mid/composicao/emissao_cli.h>
#include <nl/mid/composicao/politica_emissao.h>

#include <chrono>

#include <QFile>
#include <QIODevice>
#include <QList>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QTemporaryDir>
#include <QtTest>

class TstAncorasEmissao : public QObject
{
    Q_OBJECT

private slots:
    // Fora de Homologação, o A1 usa o conjunto de produção do bundle, e caminhoAncoras é ignorado.
    void ancorasPadraoSaoAsRaizesIcp();
    // Sem [sefin] ancoras_tls, o servidor TLS usa as CAs do sistema; com o PEM, usa só o PEM.
    void ancorasTlsPadraoDoSistema();
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
    const QString caminho = pasta.filePath(QStringLiteral("tls.pem"));
    QFile arquivo(caminho);
    QVERIFY(arquivo.open(QIODevice::WriteOnly));
    arquivo.write(padrao.first().toPem());
    arquivo.close();
    pedido.caminhoAncorasTls = caminho;
    const QList<QSslCertificate> doPem = nl::mid::composicao::ancorasTlsDoPedido(pedido);
    QCOMPARE(doPem.size(), qsizetype(1));
    QVERIFY(doPem.first() == padrao.first());
}

QTEST_APPLESS_MAIN(TstAncorasEmissao)
#include "tst_ancoras_emissao.moc"
