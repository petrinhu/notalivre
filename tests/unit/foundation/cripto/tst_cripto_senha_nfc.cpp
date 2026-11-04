// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes da forma canônica da senha (NL-3.2b, ADR-0003, entrada canônica; NIST SP 800-63B-4 para a normalização).
// Os bytes esperados são os de code points do Unicode, escritos à mão: NFC de "açã" é a, U+00E7, U+00E3; NFD é a,
// c, U+0327, a, U+0303. A fixture não sai do código sob teste.

#include <nl/foundation/cripto/senha_nfc.h>
#include <nl/foundation/cripto/senha_nfc_interno.h>

#include <QByteArray>
#include <QList>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <span>

namespace cr = nl::foundation::cripto;
namespace detalhe = nl::foundation::cripto::detalhe;

namespace {

// "açã" em NFC: a, U+00E7 (c3 a7), U+00E3 (c3 a3).
const QByteArray kAcaNfcEsperado = QByteArray::fromHex("61c3a7c3a3");
// "açã" em NFD: a, c, U+0327 (cc a7), a, U+0303 (cc 83).
const QByteArray kAcaNfd = QByteArray::fromHex("6163cca761cc83");

std::span<unsigned char> areaDe(QByteArray& buffer)
{
    return {reinterpret_cast<unsigned char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
}

QString codigoDe(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstCriptoSenhaNfc : public QObject
{
    Q_OBJECT

private slots:
    void senhaNfcENfdDaoAMesmaChave();
    void senhaAsciiNaoPassaPorQString();
    void utf8InvalidoRecusado();
    void saidaNaoApareceNoErro();
    void senhaNfcDestinoPequenoRecusado();
};

void TstCriptoSenhaNfc::senhaNfcENfdDaoAMesmaChave()
{
    QByteArray saidaNfc(16, '\0');
    QByteArray saidaNfd(16, '\0');

    const auto daNfc = cr::senhaEmNfc(kAcaNfcEsperado, areaDe(saidaNfc));
    QVERIFY2(daNfc.has_value(), "a forma NFC já canônica não pode falhar");
    QCOMPARE(*daNfc, kAcaNfcEsperado.size());
    QCOMPARE(saidaNfc.left(*daNfc), kAcaNfcEsperado);

    const auto daNfd = cr::senhaEmNfc(kAcaNfd, areaDe(saidaNfd));
    QVERIFY2(daNfd.has_value(), "a forma NFD precisa ser normalizada, não recusada");
    QCOMPARE(*daNfd, kAcaNfcEsperado.size());
    QCOMPARE(saidaNfd.left(*daNfd), kAcaNfcEsperado);
}

void TstCriptoSenhaNfc::senhaAsciiNaoPassaPorQString()
{
    const QByteArray ascii("senha-ascii-123");
    QByteArray saida(64, '\0');
    detalhe::RelatorioNfc relatorio;
    const auto resultado = detalhe::senhaEmNfcComRelatorio(ascii, areaDe(saida), relatorio);
    QVERIFY2(resultado.has_value(), "senha só ASCII não pode falhar");
    QCOMPARE(saida.left(*resultado), ascii);
    QVERIFY2(!relatorio.passouPorQString, "senha só ASCII não pode passar por QString (ADR-0003)");

    // Controle: com byte não ASCII a conversão passa por QString, e o relatório tem de dizer isso.
    QByteArray comAcento = kAcaNfcEsperado;
    detalhe::RelatorioNfc relatorioComAcento;
    QVERIFY(detalhe::senhaEmNfcComRelatorio(comAcento, areaDe(saida), relatorioComAcento).has_value());
    QVERIFY2(relatorioComAcento.passouPorQString, "o controle com acento precisa marcar a passagem por QString");
}

void TstCriptoSenhaNfc::utf8InvalidoRecusado()
{
    // Sequência truncada, forma longa de "/" (C0 AF), substituto (U+D800, ED A0 80) e acima de U+10FFFF (F4 90 80 80).
    const QList<QByteArray> invalidas = {
        QByteArray::fromHex("c3"),
        QByteArray::fromHex("c0af"),
        QByteArray::fromHex("eda080"),
        QByteArray::fromHex("f4908080"),
    };
    for (const QByteArray& entrada : invalidas) {
        QByteArray saida(64, '\0');
        const auto resultado = cr::senhaEmNfc(entrada, areaDe(saida));
        QVERIFY2(!resultado.has_value(), qPrintable(QStringLiteral("UTF-8 inválido aceito: %1").arg(QString::fromLatin1(entrada.toHex()))));
        QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.utf8_invalido"));
    }
}

void TstCriptoSenhaNfc::saidaNaoApareceNoErro()
{
    // A mensagem de erro nunca carrega os bytes recebidos (ADR-0005): o segredo "segredo-xyz" não pode sair no texto.
    const QByteArray entrada = QByteArray("segredo-xyz") + QByteArray::fromHex("c3");
    QByteArray saida(64, '\0');
    const auto resultado = cr::senhaEmNfc(entrada, areaDe(saida));
    QVERIFY(!resultado.has_value());
    const QString mensagem = QString::fromStdString(resultado.error().mensagem());
    QVERIFY2(!mensagem.contains(QStringLiteral("segredo-xyz")), "a mensagem de erro vazou um trecho da entrada");
    QVERIFY2(!mensagem.contains(QStringLiteral("c3")), "a mensagem de erro vazou o byte inválido");
}

void TstCriptoSenhaNfc::senhaNfcDestinoPequenoRecusado()
{
    // "açã" ocupa 5 bytes em NFC; destino de 3 bytes tem de recusar com cripto.tamanho, sem escrever pela metade.
    QByteArray pequeno(3, '\xAA');
    const auto resultado = cr::senhaEmNfc(kAcaNfcEsperado, areaDe(pequeno));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tamanho"));
}

QTEST_GUILESS_MAIN(TstCriptoSenhaNfc)
#include "tst_cripto_senha_nfc.moc"
