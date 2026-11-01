// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4; NL-2.10c, DA-2.66: recusas incluídas). Ver resposta_crua.h.
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
#include <utility>

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

// Nome do arquivo: 16 caracteres do hash do Id. Nem o Id nem a chave aparecem no nome.
QByteArray hashDoId(std::string_view idDps)
{
    return QCryptographicHash::hash(QByteArray(idDps.data(), static_cast<qsizetype>(idDps.size())), QCryptographicHash::Sha256)
        .toHex()
        .left(16);
}

// Uma linha do arquivo: o envelope da resposta (método, rota modelo, status, Content-Type) e o corpo pela allowlist.
QByteArray linhaDaResposta(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                           const QByteArray& corpo)
{
    const QJsonDocument mascarado = QJsonDocument::fromJson(mascararRespostaCrua(corpo));
    const QJsonObject linha{
        {QStringLiteral("metodo"), QString::fromUtf8(metodo.data(), static_cast<qsizetype>(metodo.size()))},
        {QStringLiteral("rota"), QString::fromUtf8(rota.data(), static_cast<qsizetype>(rota.size()))},
        {QStringLiteral("status"), status},
        {QStringLiteral("contentType"), QString::fromUtf8(tipoConteudo)},
        {QStringLiteral("corpo"), mascarado.object()},
    };
    return QJsonDocument(linha).toJson(QJsonDocument::Compact) + '\n';
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

GravadorRespostaCrua::GravadorRespostaCrua(QString pastaBase, std::string idDps)
    : pastaBase_(std::move(pastaBase)), idDps_(std::move(idDps))
{
}

void GravadorRespostaCrua::registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                                     const QByteArray& corpo)
{
    if (erro_) {
        return;  // Depois do primeiro erro o arquivo não recebe mais linhas, para não sair uma linha pela metade.
    }
    const QString pasta = pastaBase_ + QStringLiteral("/respostas");
    if (!QDir().mkpath(pasta)) {
        erro_ = nl::base::Erro("analise.pasta", std::nullopt, "não foi possível criar a pasta da resposta para análise");
        return;
    }
    if (caminho_.isEmpty()) {
        caminho_ = pasta + QStringLiteral("/") + QString::fromLatin1(hashDoId(idDps_)) + QStringLiteral(".jsonl");
    }
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta");
        return;
    }
    arquivo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);  // 0600 logo após a abertura, antes de qualquer linha
    const QByteArray linha = linhaDaResposta(metodo, rota, status, tipoConteudo, corpo);
    if (arquivo.write(linha) != linha.size() || !arquivo.flush()) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a resposta");
        return;
    }
    arquivo.close();
}

} // namespace nl::mid::composicao
