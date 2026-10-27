// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura de campos do JSON de entrada da DPS (NL-1.7): cada campo sai com o caminho pontuado dele, para o erro dizer onde está.
#ifndef NL_MID_COMPOSICAO_CAMPOS_JSON_H
#define NL_MID_COMPOSICAO_CAMPOS_JSON_H

#include <nl/base/erro.h>

#include <QByteArrayView>
#include <QJsonObject>
#include <QString>

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace nl::mid::composicao::campos {

/// Caminho pontuado de um campo ("servico.cTribNac"). Com pai vazio, o caminho é só a chave.
std::string caminhoDe(std::string_view caminhoPai, std::string_view chave);

/// Raiz da entrada: tem de ser um objeto JSON bem formado. Erro: "entrada.json_invalido".
std::expected<QJsonObject, nl::base::Erro> lerRaiz(QByteArrayView entradaJson);

/// Texto obrigatório. Chave ausente ou null: "entrada.campo_ausente". Outro tipo: "entrada.tipo_invalido".
std::expected<QString, nl::base::Erro> textoObrigatorio(const QJsonObject& pai, std::string_view caminhoPai, std::string_view chave);

/// Texto opcional: ausente ou null vira std::nullopt. Outro tipo que não texto: "entrada.tipo_invalido".
std::expected<std::optional<QString>, nl::base::Erro> textoOpcional(const QJsonObject& pai, std::string_view caminhoPai,
                                                                    std::string_view chave);

/// Inteiro obrigatório, sem fração e não negativo. Fora disso: "entrada.tipo_invalido".
std::expected<std::int64_t, nl::base::Erro> inteiroObrigatorio(const QJsonObject& pai, std::string_view caminhoPai,
                                                               std::string_view chave);

/// Objeto obrigatório (um nível aninhado). Ausente: "entrada.campo_ausente".
std::expected<QJsonObject, nl::base::Erro> objetoObrigatorio(const QJsonObject& pai, std::string_view caminhoPai,
                                                             std::string_view chave);

/// Objeto opcional: ausente ou null vira std::nullopt.
std::expected<std::optional<QJsonObject>, nl::base::Erro> objetoOpcional(const QJsonObject& pai, std::string_view caminhoPai,
                                                                         std::string_view chave);

} // namespace nl::mid::composicao::campos

#endif // NL_MID_COMPOSICAO_CAMPOS_JSON_H
