// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4). Ver resposta_crua.h.
#include <nl/mid/composicao/resposta_crua.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <cstddef>
#include <optional>

namespace nl::mid::composicao {
namespace {

// A chave de acesso sai com # no lugar de cada caractere: ela não vai ao arquivo em claro (ADR-0005).
const QString kCampoChave = QStringLiteral("chaveAcesso");

QJsonObject mascararObjeto(const QJsonObject& objeto);

// Allowlist de valor (CISO N-03, ADR-0005): só a chave sai mascarada; qualquer outro valor sai como <tipo:tamanho>. Os nomes dos
// campos e a forma do objeto ficam, para o analista ver o que a Sefin mandou sem ver o conteúdo.
QJsonValue mascararValor(const QString& nome, const QJsonValue& valor)
{
    if (valor.isObject()) {
        return mascararObjeto(valor.toObject());
    }
    if (valor.isArray()) {
        return QStringLiteral("<array:%1>").arg(valor.toArray().size());
    }
    if (valor.isString()) {
        const QString texto = valor.toString();
        if (nome == kCampoChave) {
            QString mascara;
            mascara.fill(QLatin1Char('#'), texto.size());
            return mascara;
        }
        return QStringLiteral("<string:%1>").arg(texto.size());
    }
    if (valor.isBool()) {
        return QStringLiteral("<bool:%1>").arg(valor.toBool() ? 4 : 5);
    }
    if (valor.isDouble()) {
        return QStringLiteral("<number:%1>").arg(QString::number(valor.toDouble()).size());
    }
    return QStringLiteral("<null:0>");
}

QJsonObject mascararObjeto(const QJsonObject& objeto)
{
    QJsonObject mascarado;
    for (auto it = objeto.constBegin(); it != objeto.constEnd(); ++it) {
        mascarado.insert(it.key(), mascararValor(it.key(), it.value()));
    }
    return mascarado;
}

} // namespace

QByteArray mascararRespostaCrua(const QByteArray& corpo)
{
    QJsonParseError erro;
    const QJsonDocument doc = QJsonDocument::fromJson(corpo, &erro);
    if (!doc.isObject()) {
        return QByteArrayLiteral("{\"aviso\":\"corpo que nao e JSON, omitido\"}");
    }
    return QJsonDocument(mascararObjeto(doc.object())).toJson(QJsonDocument::Compact);
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
