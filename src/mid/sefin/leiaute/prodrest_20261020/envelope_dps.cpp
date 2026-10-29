// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Envelope do POST /nfse (NL-2.4, DA-2.4). INFERÊNCIA até a NL-2.10: JSON {"dpsXmlGZipB64": Base64(gzip)} com
// application/json. Só codifica e decodifica; quem comprime é o gzip da Foundation.

#include <nl/mid/sefin/leiaute/prodrest_20261020/envelope_dps.h>

#include <nl/base/erro.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QString>

#include <expected>
#include <optional>

namespace nl::mid::sefin::prodrest_20261020 {
namespace {

QString chaveDoEnvelope()
{
    return QString::fromLatin1(kChaveEnvelopeDps.data(), static_cast<qsizetype>(kChaveEnvelopeDps.size()));
}

QByteArray contentTypeDoEnvelope()
{
    return QByteArray(kContentTypeEnvelopeDps.data(), static_cast<qsizetype>(kContentTypeEnvelopeDps.size()));
}

} // namespace

std::expected<EnvelopeDps, nl::base::Erro> montarEnvelopeDps(const QByteArray& gzipDoXmlAssinado)
{
    if (gzipDoXmlAssinado.isEmpty()) {
        return std::unexpected(nl::base::Erro{"envelope.vazio", std::nullopt, "gzip do XML assinado vazio"});
    }
    QJsonObject objeto;
    objeto.insert(chaveDoEnvelope(), QString::fromLatin1(gzipDoXmlAssinado.toBase64()));

    EnvelopeDps envelope;
    envelope.corpo = QJsonDocument(objeto).toJson(QJsonDocument::Compact);
    envelope.contentType = contentTypeDoEnvelope();
    return envelope;
}

std::expected<QByteArray, nl::base::Erro> lerEnvelopeDps(const QByteArray& corpo)
{
    QJsonParseError erro{};
    const QJsonDocument documento = QJsonDocument::fromJson(corpo, &erro);
    if (erro.error != QJsonParseError::NoError || !documento.isObject()) {
        return std::unexpected(nl::base::Erro{"envelope.json", std::nullopt, "corpo do envelope não é JSON objeto"});
    }
    const QJsonObject objeto = documento.object();
    if (objeto.size() != 1 || !objeto.contains(chaveDoEnvelope())) {
        return std::unexpected(nl::base::Erro{"envelope.chave", std::nullopt, "envelope sem a chave da pasta do leiaute"});
    }
    // Decodificação estrita: fromBase64 padrão ignora caracteres inválidos e aceitaria o corpo em silêncio (revisão O4).
    const QByteArray::FromBase64Result decodificado =
        QByteArray::fromBase64Encoding(objeto.value(chaveDoEnvelope()).toString().toLatin1());
    if (!decodificado) {
        return std::unexpected(nl::base::Erro{"envelope.base64", std::nullopt, "envelope com Base64 inválido"});
    }
    const QByteArray gzip = decodificado.decoded;
    if (gzip.isEmpty()) {
        return std::unexpected(nl::base::Erro{"envelope.vazio", std::nullopt, "envelope sem gzip"});
    }
    return gzip;
}

} // namespace nl::mid::sefin::prodrest_20261020
