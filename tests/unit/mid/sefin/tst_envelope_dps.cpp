// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do envelope da DPS (NL-2.4, DA-2.4). Tudo aqui é INFERÊNCIA até a NL-2.10: o formato vem de fontes secundárias
// (docs/pesquisa/onda2_api_sefin.md, seção 1.2), não de manual oficial. O comentário de cada teste diz isso.
// A chave e o Content-Type vêm da pasta do leiaute (envelope_dps.h); a conformidade do GZip fica no teste de compressão.

#include <nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QtTest>

#include <cstddef>
#include <string>

namespace prod = nl::mid::sefin::prodrest_20261020;

namespace {

/// Carga de teste com bytes de 0x00 a 0xFF repetidos, para o Base64 passar pelo alfabeto inteiro.
QByteArray cargaDeTeste(std::size_t tamanho)
{
    QByteArray carga;
    carga.reserve(static_cast<qsizetype>(tamanho));
    for (std::size_t indice = 0; indice < tamanho; ++indice) {
        carga.append(static_cast<char>(indice % 256));
    }
    return carga;
}

/// Chave do JSON, vinda da constante da pasta do leiaute.
QString chaveDoEnvelope()
{
    return QString::fromLatin1(prod::kChaveEnvelopeDps.data(), static_cast<qsizetype>(prod::kChaveEnvelopeDps.size()));
}

/// Corpo lido como JSON (nunca por busca de texto).
QJsonObject objetoDoCorpo(const QByteArray& corpo)
{
    QJsonParseError erro{};
    const QJsonDocument documento = QJsonDocument::fromJson(corpo, &erro);
    if (erro.error != QJsonParseError::NoError) {
        return {};
    }
    return documento.object();
}

} // namespace

class TstEnvelopeDps : public QObject
{
    Q_OBJECT

private slots:
    void envelopeIdaEVolta();
    void base64PadraoSemQuebraDeLinha();
    void chaveDoJsonVemDaPastaDoLeiaute();
    void contentTypeEhApplicationJson();
    void envelopeVazioRecusado();
};

void TstEnvelopeDps::envelopeIdaEVolta()
{
    // INFERÊNCIA até a NL-2.10: o corpo é {"dpsXmlGZipB64": Base64(carga)} e volta à carga original.
    // Aqui a carga são bytes arbitrários: a conformidade do GZip fica no teste de compressão, que ainda não existe.
    const QByteArray carga = cargaDeTeste(1000);
    const auto envelope = prod::montarEnvelopeDps(carga);
    QVERIFY2(envelope.has_value(), "montarEnvelopeDps recusou uma carga válida");

    const auto lido = prod::lerEnvelopeDps(envelope->corpo);
    QVERIFY2(lido.has_value(), "lerEnvelopeDps não leu o corpo que montarEnvelopeDps gerou");
    QCOMPARE(*lido, carga);
}

void TstEnvelopeDps::base64PadraoSemQuebraDeLinha()
{
    // INFERÊNCIA até a NL-2.10: Base64 padrão (alfabeto com + e /, padding com =), sem quebra de linha.
    // 1000 bytes não é múltiplo de 3, então o Base64 tem de terminar com padding.
    const auto envelope = prod::montarEnvelopeDps(cargaDeTeste(1000));
    QVERIFY2(envelope.has_value(), "montarEnvelopeDps recusou uma carga válida");

    const QString valor = objetoDoCorpo(envelope->corpo).value(chaveDoEnvelope()).toString();
    QVERIFY2(!valor.isEmpty(), "a chave do envelope não trouxe Base64");
    QVERIFY2(!valor.contains(QLatin1Char('\n')) && !valor.contains(QLatin1Char('\r')),
             "o Base64 do envelope tem quebra de linha");
    QVERIFY2(valor.size() % 4 == 0, "o Base64 não tem tamanho múltiplo de 4");
    QVERIFY2(valor.endsWith(QLatin1Char('=')), "o Base64 devia ter padding com = para 1000 bytes");
    for (const QChar caractere : valor) {
        const char16_t codigo = caractere.unicode();
        const bool letraOuDigito = (codigo >= u'A' && codigo <= u'Z') || (codigo >= u'a' && codigo <= u'z')
                                   || (codigo >= u'0' && codigo <= u'9');
        const bool simbolo = codigo == u'+' || codigo == u'/' || codigo == u'=';
        QVERIFY2(letraOuDigito || simbolo, "caractere fora do alfabeto Base64 padrão");
    }
}

void TstEnvelopeDps::chaveDoJsonVemDaPastaDoLeiaute()
{
    // INFERÊNCIA até a NL-2.10: o JSON tem uma única chave, a que a pasta do leiaute declara (kChaveEnvelopeDps).
    const auto envelope = prod::montarEnvelopeDps(cargaDeTeste(16));
    QVERIFY2(envelope.has_value(), "montarEnvelopeDps recusou uma carga válida");

    const QJsonObject objeto = objetoDoCorpo(envelope->corpo);
    QCOMPARE(objeto.keys().size(), qsizetype{1});
    QVERIFY2(objeto.contains(chaveDoEnvelope()), "o envelope não traz a chave da pasta do leiaute");
}

void TstEnvelopeDps::contentTypeEhApplicationJson()
{
    // INFERÊNCIA até a NL-2.10: nenhuma fonte oficial lida informa o Content-Type; as secundárias dizem application/json.
    const auto envelope = prod::montarEnvelopeDps(cargaDeTeste(16));
    QVERIFY2(envelope.has_value(), "montarEnvelopeDps recusou uma carga válida");
    QCOMPARE(envelope->contentType, QByteArray("application/json"));
}

void TstEnvelopeDps::envelopeVazioRecusado()
{
    // Gzip vazio nunca é DPS: a montagem recusa com o código envelope.vazio.
    const auto envelope = prod::montarEnvelopeDps(QByteArray{});
    QVERIFY2(!envelope.has_value(), "montarEnvelopeDps aceitou um gzip vazio");
    QCOMPARE(QString::fromStdString(envelope.error().codigo()), QStringLiteral("envelope.vazio"));
}

QTEST_GUILESS_MAIN(TstEnvelopeDps)
#include "tst_envelope_dps.moc"
