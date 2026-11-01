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

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

namespace nl::mid::composicao {
namespace {

// A chave de acesso sai com # no lugar de cada caractere: ela não vai ao arquivo em claro (ADR-0005).
const QString kCampoChave = QStringLiteral("chaveAcesso");
// Os únicos campos onde o código E pode sair sem máscara (condição do CISO, 31/10/2026, emenda 1 item c).
const QString kCampoCodigo = QStringLiteral("Codigo");
const QString kCampoCodigoMinusculo = QStringLiteral("codigo");

// Limites da descida (condição do CISO, 31/10/2026): oito níveis de objeto ou array; cem elementos por array.
constexpr int kProfundidadeMaxima = 8;
constexpr qsizetype kElementosMaximos = 100;
// Teto do arquivo de análise por emissão (condição do CISO): 4 MiB. A linha de corte fica fora do teto.
constexpr qsizetype kTetoArquivoBytes = 4 * 1024 * 1024;

QJsonObject mascararObjeto(const QJsonObject& objeto, int nivel);
QJsonValue mascararValor(const QString& nome, const QJsonValue& valor, int nivel);

// Código E válido: ^E\d{3,4}$ (DA-2.41, b). A mesma regra existe em cliente_sefin.cpp, que é privada dele (segunda ocorrência).
bool codigoEValido(const QString& texto)
{
    if (texto.size() != 4 && texto.size() != 5) {
        return false;
    }
    if (texto.at(0) != QLatin1Char('E')) {
        return false;
    }
    for (qsizetype i = 1; i < texto.size(); ++i) {
        if (texto.at(i) < QLatin1Char('0') || texto.at(i) > QLatin1Char('9')) {
            return false;
        }
    }
    return true;
}

// Um texto sai inteiro só se o campo é Codigo ou codigo e o valor inteiro casa com ^E\d{3,4}$.
bool campoDeCodigoE(const QString& nome, const QString& texto)
{
    return (nome == kCampoCodigo || nome == kCampoCodigoMinusculo) && codigoEValido(texto);
}

// Array: cada um dos cem primeiros elementos passa pela mesma regra; o resto vira um marcador <mais:N>.
QJsonArray mascararArray(const QJsonArray& array, int nivel)
{
    QJsonArray mascarado;
    const qsizetype visiveis = std::min(array.size(), kElementosMaximos);
    for (qsizetype i = 0; i < visiveis; ++i) {
        mascarado.append(mascararValor(QString(), array.at(i), nivel + 1));
    }
    if (array.size() > visiveis) {
        mascarado.append(QStringLiteral("<mais:%1>").arg(array.size() - visiveis));
    }
    return mascarado;
}

// Allowlist de valor (CISO N-03, ADR-0005): só a chave e o código E válido saem; qualquer outro valor sai como <tipo:tamanho>. Os
// nomes dos campos e a forma do objeto ficam, para o analista ver o que a Sefin mandou sem ver o conteúdo. O nível é o do valor:
// a raiz é 1, e um objeto ou array no nível 9 ou abaixo vira <profundidade>.
QJsonValue mascararValor(const QString& nome, const QJsonValue& valor, int nivel)
{
    if (valor.isObject()) {
        if (nivel > kProfundidadeMaxima) {
            return QStringLiteral("<profundidade>");
        }
        return mascararObjeto(valor.toObject(), nivel);
    }
    if (valor.isArray()) {
        if (nivel > kProfundidadeMaxima) {
            return QStringLiteral("<profundidade>");
        }
        return mascararArray(valor.toArray(), nivel);
    }
    if (valor.isString()) {
        const QString texto = valor.toString();
        if (nome == kCampoChave) {
            QString mascara;
            mascara.fill(QLatin1Char('#'), texto.size());
            return mascara;
        }
        if (campoDeCodigoE(nome, texto)) {
            return texto;
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

QJsonObject mascararObjeto(const QJsonObject& objeto, int nivel)
{
    QJsonObject mascarado;
    for (auto it = objeto.constBegin(); it != objeto.constEnd(); ++it) {
        mascarado.insert(it.key(), mascararValor(it.key(), it.value(), nivel + 1));
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
    return QJsonDocument(mascararObjeto(doc.object(), 1)).toJson(QJsonDocument::Compact);
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
    if (cortado_) {
        ++omitidas_;
        escreverCorte();
        return;
    }
    const QByteArray linha = linhaDaResposta(metodo, rota, status, tipoConteudo, corpo);
    if (tamanhoRespostas_ + linha.size() > kTetoArquivoBytes) {
        cortado_ = true;  // a resposta inteira não cabe: nenhuma linha é cortada pela metade
        ++omitidas_;
        escreverCorte();
        return;
    }
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Append)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta");
        return;
    }
    arquivo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);  // 0600 logo após a abertura, antes de qualquer linha
    if (arquivo.write(linha) != linha.size() || !arquivo.flush()) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a resposta");
        return;
    }
    arquivo.close();
    tamanhoRespostas_ += linha.size();
}

void GravadorRespostaCrua::escreverCorte()
{
    // Corta o arquivo no fim das respostas e escreve a linha de corte com o total atual: uma linha só, nunca duas com N defasado.
    QFile arquivo(caminho_);
    if (!arquivo.open(QIODevice::ReadWrite)) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível abrir o arquivo da resposta");
        return;
    }
    arquivo.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    const QByteArray corte = QByteArrayLiteral("{\"corte\":\"teto de ") + QByteArray::number(kTetoArquivoBytes)
                             + QByteArrayLiteral(" bytes atingido\",\"respostas_omitidas\":") + QByteArray::number(omitidas_)
                             + QByteArrayLiteral("}\n");
    if (!arquivo.resize(tamanhoRespostas_) || !arquivo.seek(tamanhoRespostas_) || arquivo.write(corte) != corte.size()
        || !arquivo.flush()) {
        erro_ = nl::base::Erro("analise.gravacao", std::nullopt, "não foi possível gravar a linha de corte");
    }
}

} // namespace nl::mid::composicao
