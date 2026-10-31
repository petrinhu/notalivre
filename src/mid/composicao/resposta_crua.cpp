// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4). Ver resposta_crua.h.
#include <nl/mid/composicao/resposta_crua.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <cstddef>
#include <optional>

namespace nl::mid::composicao {

QByteArray mascararRespostaCrua(const QByteArray& corpo)
{
    QJsonParseError erro;
    const QJsonDocument doc = QJsonDocument::fromJson(corpo, &erro);
    if (!doc.isObject()) {
        return QByteArrayLiteral("{\"aviso\":\"corpo que nao e JSON, omitido\"}");
    }
    QJsonObject objeto = doc.object();
    if (objeto.contains(QStringLiteral("nfseXmlGZipB64"))) {
        objeto[QStringLiteral("nfseXmlGZipB64")] = QStringLiteral("<omitido: XML da NFS-e com dados do titular>");
    }
    if (objeto.contains(QStringLiteral("chaveAcesso"))) {
        QString mascara;
        mascara.fill(QLatin1Char('#'), objeto.value(QStringLiteral("chaveAcesso")).toString().size());
        objeto[QStringLiteral("chaveAcesso")] = mascara;
    }
    return QJsonDocument(objeto).toJson(QJsonDocument::Compact);
}

std::expected<QString, nl::base::Erro> gravarRespostaCrua(const QByteArray& corpo, const QString& pastaBase, std::string_view idDps)
{
    const QString pasta = pastaBase + QStringLiteral("/respostas");
    if (!QDir().mkpath(pasta)) {
        return std::unexpected(nl::base::Erro("analise.pasta", std::nullopt, "não foi possível criar a pasta da resposta para análise"));
    }
    const QByteArray hash = QCryptographicHash::hash(QByteArray(idDps.data(), static_cast<qsizetype>(idDps.size())),
                                                     QCryptographicHash::Sha256)
                                .toHex()
                                .left(16);
    const QString caminho = pasta + QStringLiteral("/") + QString::fromLatin1(hash) + QStringLiteral(".json");
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return std::unexpected(nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta"));
    }
    arquivo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);  // 0600 antes de qualquer byte do corpo
    if (arquivo.write(mascararRespostaCrua(corpo)) < 0 || !arquivo.flush()) {
        return std::unexpected(nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a resposta"));
    }
    arquivo.close();
    return caminho;
}

} // namespace nl::mid::composicao
