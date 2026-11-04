// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do AES-256-GCM da Foundation de cripto (NL-3.2b, ADR-0003 regra 1). O vetor vem de fonte pública:
// McGrew e Viega, "The Galois/Counter Mode of Operation (GCM)", Test Case 16, p. 39 (NIST gcm-spec.pdf, lido em 04/11/2026,
// https://csrc.nist.rip/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-spec.pdf). Os valores do teste são os
// impressos no documento, nunca gerados pelo código sob teste.

#include <nl/foundation/cripto/aead.h>
#include <nl/foundation/cripto/tamanhos.h>

#include <QByteArray>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <expected>
#include <span>

namespace cr = nl::foundation::cripto;

namespace {

// Test Case 16 do GCM (AES-256, 60 bytes de texto, 20 bytes de AAD, IV de 96 bits).
const QByteArray kChave16 = QByteArray::fromHex(
    "feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308");
const QByteArray kIv16 = QByteArray::fromHex("cafebabefacedbaddecaf888");
const QByteArray kAad16 = QByteArray::fromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2");
const QByteArray kClaro16 = QByteArray::fromHex(
    "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39");
const QByteArray kCifra16 = QByteArray::fromHex(
    "522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662");
const QByteArray kTag16 = QByteArray::fromHex("76fc6ece0f4e1768cddf8853bb2d551b");

std::span<unsigned char> areaDe(QByteArray& buffer)
{
    return {reinterpret_cast<unsigned char*>(buffer.data()), static_cast<std::size_t>(buffer.size())};
}

QString codigoDe(const nl::base::Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstCriptoAead : public QObject
{
    Q_OBJECT

private slots:
    void gcmVetorMcgrewViega16();
    void tagAdulteradaReprova();
    void aadTrocadaReprova();
    void nonceTrocadoReprova();
    void textoMenorQueATagReprova();
    void tagTruncadaReprova();
    void falhaDeTagDeixaSaidaZerada();
    void nonceDeTamanhoErradoReprova();
    void chaveDeTamanhoErradoReprova();
    void claroVazioComAadTagExata();
    void claroVazioComAadDecifraRetornoZero();
};

// Vetor público do NIST CAVP gcmEncryptExtIV256 (PTlen = 0, AADlen = 16, chave de 256 bits). Lido nas fontes secundárias
// que citam o CAVP, sem o zip do NIST: openHiTLS, autoverificação CMVP, https://github.com/openHiTLS/openHiTLS/blob/1b57ef13075732ac4d2d35c69c0cef88bd69e8e4/crypto/provider/src/cmvp/cmvp_utils/cmvp_selftest_mac.c#L174-L183
// e Tink, https://fuchsia.googlesource.com/third_party/tink/+/5d4695d445c3231b0181c70b6694b410710967f6/javascript/subtle/aes_gcm_test.js
// O valor não sai da implementação deste projeto.
namespace {
const QByteArray kChaveClaroVazio = QByteArray::fromHex("78dc4e0aaf52d935c3c01eea57428f00ca1fd475f5da86a49c8dd73d68c8e223");
const QByteArray kIvClaroVazio = QByteArray::fromHex("d79cf22d504cc793c3fb6c8a");
const QByteArray kAadClaroVazio = QByteArray::fromHex("b96baa8c1c75a671bfb2d08d06be5f36");
const QByteArray kTagClaroVazio = QByteArray::fromHex("3e5d486aa2e30b22e040b85723a06e76");
} // namespace

void TstCriptoAead::claroVazioComAadTagExata()
{
    const auto cifrado = cr::cifrarAes256Gcm(kChaveClaroVazio, kIvClaroVazio, kAadClaroVazio, QByteArray());
    QVERIFY2(cifrado.has_value(), "cifrar claro vazio com AAD não pode falhar");
    QCOMPARE(cifrado->textoComTag.size(), static_cast<qsizetype>(cr::kTamanhoTag));
    QCOMPARE(cifrado->textoComTag, kTagClaroVazio);
}

void TstCriptoAead::claroVazioComAadDecifraRetornoZero()
{
    QByteArray claro;
    const auto decifrado = cr::decifrarAes256Gcm(kChaveClaroVazio, kIvClaroVazio, kAadClaroVazio, kTagClaroVazio, areaDe(claro));
    QVERIFY2(decifrado.has_value(), "decifrar só a tag com AAD correta não pode falhar");
    QCOMPARE(*decifrado, 0);

    QByteArray aadTrocada = kAadClaroVazio;
    aadTrocada[0] = static_cast<char>(aadTrocada[0] ^ 0x01);
    const auto reprova = cr::decifrarAes256Gcm(kChaveClaroVazio, kIvClaroVazio, aadTrocada, kTagClaroVazio, areaDe(claro));
    QVERIFY(!reprova.has_value());
    QCOMPARE(codigoDe(reprova.error()), QStringLiteral("cripto.tag"));
}

void TstCriptoAead::gcmVetorMcgrewViega16()
{
    const auto cifrado = cr::cifrarAes256Gcm(kChave16, kIv16, kAad16, kClaro16);
    QVERIFY2(cifrado.has_value(), "cifrar o vetor 16 do GCM não pode falhar");
    QCOMPARE(cifrado->nonce, kIv16);
    QCOMPARE(cifrado->textoComTag, kCifra16 + kTag16);

    QByteArray claro(kClaro16.size(), '\0');
    const auto decifrado = cr::decifrarAes256Gcm(kChave16, kIv16, kAad16, cifrado->textoComTag, areaDe(claro));
    QVERIFY2(decifrado.has_value(), "decifrar o vetor 16 do GCM não pode falhar");
    QCOMPARE(*decifrado, kClaro16.size());
    QCOMPARE(claro, kClaro16);
}

void TstCriptoAead::tagAdulteradaReprova()
{
    QByteArray textoComTag = kCifra16 + kTag16;
    textoComTag[textoComTag.size() - 1] = static_cast<char>(textoComTag[textoComTag.size() - 1] ^ 0x01);

    QByteArray claro(kClaro16.size(), '\0');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, kIv16, kAad16, textoComTag, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tag"));
}

void TstCriptoAead::aadTrocadaReprova()
{
    QByteArray aadTrocada = kAad16;
    aadTrocada[0] = static_cast<char>(aadTrocada[0] ^ 0x01);

    QByteArray claro(kClaro16.size(), '\0');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, kIv16, aadTrocada, kCifra16 + kTag16, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tag"));
}

void TstCriptoAead::nonceTrocadoReprova()
{
    QByteArray nonceTrocado = kIv16;
    nonceTrocado[11] = static_cast<char>(nonceTrocado[11] ^ 0x01);

    QByteArray claro(kClaro16.size(), '\0');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, nonceTrocado, kAad16, kCifra16 + kTag16, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tag"));
}

void TstCriptoAead::textoMenorQueATagReprova()
{
    const QByteArray menorQueATag(cr::kTamanhoTag - 1, '\x5a');
    QByteArray claro(16, '\xAA');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, kIv16, kAad16, menorQueATag, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tamanho"));
    QCOMPARE(claro, QByteArray(16, '\0'));
}

void TstCriptoAead::tagTruncadaReprova()
{
    // Tag com um byte a menos: a conferência lê 15 bytes de tag e um byte de texto, e a tag tem de falhar.
    QByteArray textoComTagTruncada = kCifra16 + kTag16;
    textoComTagTruncada.chop(1);

    QByteArray claro(kClaro16.size(), '\0');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, kIv16, kAad16, textoComTagTruncada, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tag"));
}

void TstCriptoAead::falhaDeTagDeixaSaidaZerada()
{
    QByteArray textoComTag = kCifra16 + kTag16;
    textoComTag[textoComTag.size() - 1] = static_cast<char>(textoComTag[textoComTag.size() - 1] ^ 0x80);

    // O claro é escrito antes da conferência da tag. Em falha, a área volta toda a zero.
    QByteArray claro(kClaro16.size(), '\xAA');
    const auto resultado = cr::decifrarAes256Gcm(kChave16, kIv16, kAad16, textoComTag, areaDe(claro));
    QVERIFY(!resultado.has_value());
    QCOMPARE(codigoDe(resultado.error()), QStringLiteral("cripto.tag"));
    QCOMPARE(claro, QByteArray(kClaro16.size(), '\0'));
}

void TstCriptoAead::nonceDeTamanhoErradoReprova()
{
    const QByteArray nonceCurto(cr::kTamanhoNonce - 1, '\x01');
    const auto cifrado = cr::cifrarAes256Gcm(kChave16, nonceCurto, kAad16, kClaro16);
    QVERIFY(!cifrado.has_value());
    QCOMPARE(codigoDe(cifrado.error()), QStringLiteral("cripto.tamanho"));

    QByteArray claro(kClaro16.size(), '\0');
    const auto decifrado = cr::decifrarAes256Gcm(kChave16, nonceCurto, kAad16, kCifra16 + kTag16, areaDe(claro));
    QVERIFY(!decifrado.has_value());
    QCOMPARE(codigoDe(decifrado.error()), QStringLiteral("cripto.tamanho"));
}

void TstCriptoAead::chaveDeTamanhoErradoReprova()
{
    const QByteArray chaveCurta(cr::kTamanhoChave - 1, '\x02');
    const auto cifrado = cr::cifrarAes256Gcm(chaveCurta, kIv16, kAad16, kClaro16);
    QVERIFY(!cifrado.has_value());
    QCOMPARE(codigoDe(cifrado.error()), QStringLiteral("cripto.tamanho"));

    QByteArray claro(kClaro16.size(), '\0');
    const auto decifrado = cr::decifrarAes256Gcm(chaveCurta, kIv16, kAad16, kCifra16 + kTag16, areaDe(claro));
    QVERIFY(!decifrado.has_value());
    QCOMPARE(codigoDe(decifrado.error()), QStringLiteral("cripto.tamanho"));
}

QTEST_GUILESS_MAIN(TstCriptoAead)
#include "tst_cripto_aead.moc"
