// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor do corpo de erro da Sefin (NL-2.8, pesquisa g: INFERÊNCIA até a NL-2.10). Aceita "erro" (objeto ou lista) e
// "erros" (lista), com "Codigo" ou "codigo". Só o código sai daqui; a descrição nunca é lida (ADR-0005 c).
#include <nl/mid/sefin/cliente/leitor_erro_sefin.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace nl::mid::sefin::cliente {
namespace {

// Código de um objeto de erro: "Codigo" ou "codigo", o que vier como texto não vazio.
std::optional<std::string> codigoDoObjeto(const QJsonObject& objeto)
{
    for (const char* chave : {"Codigo", "codigo"}) {
        const QJsonValue valor = objeto.value(QLatin1StringView(chave));
        if (valor.isString() && !valor.toString().isEmpty()) {
            return valor.toString().toStdString();
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<std::string> codigoDoErro(const QByteArray& corpo)
{
    const QJsonDocument documento = QJsonDocument::fromJson(corpo);
    if (!documento.isObject()) {
        return std::nullopt;
    }
    const QJsonObject raiz = documento.object();
    const QJsonValue erro = raiz.value(QStringLiteral("erro"));
    if (erro.isObject()) {
        if (auto codigo = codigoDoObjeto(erro.toObject())) {
            return codigo;
        }
    } else if (erro.isArray()) {
        for (const QJsonValue& item : erro.toArray()) {
            if (item.isObject()) {
                if (auto codigo = codigoDoObjeto(item.toObject())) {
                    return codigo;
                }
            }
        }
    }
    const QJsonValue erros = raiz.value(QStringLiteral("erros"));
    if (erros.isArray()) {
        for (const QJsonValue& item : erros.toArray()) {
            if (item.isObject()) {
                if (auto codigo = codigoDoObjeto(item.toObject())) {
                    return codigo;
                }
            }
        }
    }
    return codigoDoObjeto(raiz);
}

} // namespace nl::mid::sefin::cliente
