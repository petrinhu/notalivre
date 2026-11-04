// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes de HMAC-SHA256, HKDF-SHA256 e comparação em tempo constante (NL-3.2b). Vetores de fonte pública, com a origem:
// HMAC: RFC 4231, seção 4.2 (Test Case 1) e 4.3 (Test Case 2). HKDF: RFC 5869, apêndice A, Test Cases 1 e 3.
// Os valores são os impressos nos documentos, lidos em 04/11/2026; o código sob teste não gera nenhum deles.

#include <nl/foundation/cripto/comparacao.h>
#include <nl/foundation/cripto/hkdf.h>
#include <nl/foundation/cripto/hmac.h>

#include <QByteArray>
#include <QString>
#include <QtTest>

#include <array>
#include <cstddef>
#include <span>

namespace cr = nl::foundation::cripto;

class TstCriptoPrimitivas : public QObject
{
    Q_OBJECT

private slots:
    void hmacRfc4231();
    void hkdfRfc5869();
    void iguaisEmTempoConstanteDetectaCadaByte();
};

void TstCriptoPrimitivas::hmacRfc4231()
{
    // RFC 4231, 4.2: chave de 20 bytes 0x0b, dado "Hi There".
    const QByteArray chave1 = QByteArray::fromHex("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
    std::array<unsigned char, 32> saida1{};
    const auto resultado1 = cr::hmacSha256(chave1, QByteArray("Hi There"), saida1);
    QVERIFY2(resultado1.has_value(), "HMAC do Test Case 1 não pode falhar");
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(saida1.data()), 32).toHex(),
             QByteArray("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"));

    // RFC 4231, 4.3: chave "Jefe", dado "what do ya want for nothing?".
    std::array<unsigned char, 32> saida2{};
    const auto resultado2 =
        cr::hmacSha256(QByteArray("Jefe"), QByteArray("what do ya want for nothing?"), saida2);
    QVERIFY2(resultado2.has_value(), "HMAC do Test Case 2 não pode falhar");
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(saida2.data()), 32).toHex(),
             QByteArray("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
}

void TstCriptoPrimitivas::hkdfRfc5869()
{
    const QByteArray ikm(22, '\x0b');

    // RFC 5869, A.1: salt 0x00..0x0c, info 0xf0..0xf9, L = 42.
    const QByteArray salt1 = QByteArray::fromHex("000102030405060708090a0b0c");
    const QByteArray info1 = QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9");
    QByteArray okm1(42, '\0');
    const auto resultado1 = cr::hkdfSha256(ikm, salt1, info1,
        std::span<unsigned char>(reinterpret_cast<unsigned char*>(okm1.data()), 42));
    QVERIFY2(resultado1.has_value(), "HKDF do Test Case 1 não pode falhar");
    QCOMPARE(okm1.toHex(), QByteArray("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865"));

    // RFC 5869, A.3: salt e info vazios, L = 42.
    QByteArray okm3(42, '\0');
    const auto resultado3 = cr::hkdfSha256(ikm, QByteArray(), QByteArray(),
        std::span<unsigned char>(reinterpret_cast<unsigned char*>(okm3.data()), 42));
    QVERIFY2(resultado3.has_value(), "HKDF do Test Case 3 (salt e info vazios) não pode falhar");
    QCOMPARE(okm3.toHex(), QByteArray("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8"));
}

void TstCriptoPrimitivas::iguaisEmTempoConstanteDetectaCadaByte()
{
    const QByteArray referencia(32, '\x11');
    QVERIFY(cr::iguaisEmTempoConstante(referencia, QByteArray(referencia)));

    for (int posicao = 0; posicao < referencia.size(); ++posicao) {
        QByteArray diferente = referencia;
        diferente[posicao] = static_cast<char>(diferente[posicao] ^ 0x01);
        QVERIFY2(!cr::iguaisEmTempoConstante(referencia, diferente),
                 qPrintable(QStringLiteral("diferença na posição %1 precisa dar falso").arg(posicao)));
    }

    // Tamanhos diferentes dão falso, mesmo quando um é prefixo do outro.
    QVERIFY(!cr::iguaisEmTempoConstante(referencia, referencia.left(31)));
    QVERIFY(!cr::iguaisEmTempoConstante(referencia.left(31), referencia));
}

QTEST_GUILESS_MAIN(TstCriptoPrimitivas)
#include "tst_cripto_primitivas.moc"
