// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura de campos do JSON de entrada (NL-1.7): ausência, tipo e forma de cada campo, cada um com o caminho pontuado.
#include "campos_json.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

#include <cmath>

namespace nl::mid::composicao::campos {

namespace {

/// Maior inteiro que um double guarda sem perda (2^53). Acima disso o JSON não é um inteiro confiável.
constexpr double kMaximoInteiro = 9007199254740992.0;

QString comoQString(std::string_view texto)
{
    return QString::fromUtf8(texto.data(), static_cast<qsizetype>(texto.size()));
}

nl::base::Erro campoAusente(const std::string& caminho)
{
    return nl::base::Erro{"entrada.campo_ausente", caminho, "campo obrigatório ausente: " + caminho};
}

nl::base::Erro tipoInvalido(const std::string& caminho, std::string_view esperado)
{
    return nl::base::Erro{"entrada.tipo_invalido", caminho, "campo " + caminho + " deve ser " + std::string{esperado}};
}

QJsonValue valorDe(const QJsonObject& pai, std::string_view chave)
{
    return pai.value(comoQString(chave));
}

/// Ausente e null são o mesmo caso para o leitor: a chave não traz valor.
bool semValor(const QJsonValue& valor)
{
    return valor.isUndefined() || valor.isNull();
}

} // namespace

std::string caminhoDe(std::string_view caminhoPai, std::string_view chave)
{
    if (caminhoPai.empty()) {
        return std::string{chave};
    }
    return std::string{caminhoPai} + "." + std::string{chave};
}

std::expected<QJsonObject, nl::base::Erro> lerRaiz(QByteArrayView entradaJson)
{
    QJsonParseError erroDeLeitura{};
    const QJsonDocument documento = QJsonDocument::fromJson(entradaJson.toByteArray(), &erroDeLeitura);
    if (erroDeLeitura.error != QJsonParseError::NoError) {
        return std::unexpected(nl::base::Erro{"entrada.json_invalido", std::nullopt,
                                              "JSON malformado: " + erroDeLeitura.errorString().toStdString()});
    }
    if (!documento.isObject()) {
        return std::unexpected(
            nl::base::Erro{"entrada.json_invalido", std::nullopt, "a raiz da entrada deve ser um objeto JSON"});
    }
    return documento.object();
}

std::expected<QString, nl::base::Erro> textoObrigatorio(const QJsonObject& pai, std::string_view caminhoPai, std::string_view chave)
{
    const std::string caminho = caminhoDe(caminhoPai, chave);
    const QJsonValue valor = valorDe(pai, chave);
    if (semValor(valor)) {
        return std::unexpected(campoAusente(caminho));
    }
    if (!valor.isString()) {
        return std::unexpected(tipoInvalido(caminho, "texto"));
    }
    return valor.toString();
}

std::expected<std::optional<QString>, nl::base::Erro> textoOpcional(const QJsonObject& pai, std::string_view caminhoPai,
                                                                    std::string_view chave)
{
    const std::string caminho = caminhoDe(caminhoPai, chave);
    const QJsonValue valor = valorDe(pai, chave);
    if (semValor(valor)) {
        return std::optional<QString>{std::nullopt};
    }
    if (!valor.isString()) {
        return std::unexpected(tipoInvalido(caminho, "texto"));
    }
    return std::optional<QString>{valor.toString()};
}

std::expected<std::int64_t, nl::base::Erro> inteiroObrigatorio(const QJsonObject& pai, std::string_view caminhoPai,
                                                               std::string_view chave)
{
    const std::string caminho = caminhoDe(caminhoPai, chave);
    const QJsonValue valor = valorDe(pai, chave);
    if (semValor(valor)) {
        return std::unexpected(campoAusente(caminho));
    }
    const double numero = valor.toDouble();
    const bool inteiroNaoNegativo = valor.isDouble() && numero >= 0.0 && numero <= kMaximoInteiro && std::trunc(numero) == numero;
    if (!inteiroNaoNegativo) {
        return std::unexpected(tipoInvalido(caminho, "um inteiro não negativo"));
    }
    return static_cast<std::int64_t>(numero);
}

std::expected<QJsonObject, nl::base::Erro> objetoObrigatorio(const QJsonObject& pai, std::string_view caminhoPai,
                                                             std::string_view chave)
{
    const std::string caminho = caminhoDe(caminhoPai, chave);
    const QJsonValue valor = valorDe(pai, chave);
    if (semValor(valor)) {
        return std::unexpected(campoAusente(caminho));
    }
    if (!valor.isObject()) {
        return std::unexpected(tipoInvalido(caminho, "um objeto"));
    }
    return valor.toObject();
}

std::expected<std::optional<QJsonObject>, nl::base::Erro> objetoOpcional(const QJsonObject& pai, std::string_view caminhoPai,
                                                                         std::string_view chave)
{
    const std::string caminho = caminhoDe(caminhoPai, chave);
    const QJsonValue valor = valorDe(pai, chave);
    if (semValor(valor)) {
        return std::optional<QJsonObject>{std::nullopt};
    }
    if (!valor.isObject()) {
        return std::unexpected(tipoInvalido(caminho, "um objeto"));
    }
    return std::optional<QJsonObject>{valor.toObject()};
}

} // namespace nl::mid::composicao::campos
