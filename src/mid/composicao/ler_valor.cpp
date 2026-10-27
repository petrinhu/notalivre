// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitor de valor monetário em pt-br (NL-1.7): texto obrigatório da chave, convertido pelo Back com o caminho do campo no erro.
#include "campos_json.h"
#include "leitores_valores.h"

#include <nl/back/dinheiro/valor.h>

namespace nl::mid::composicao::leitores {

namespace nb = nl::back;

namespace {

/// O Back não sabe em que chave o texto veio, então o erro recebe o caminho completo do campo.
std::expected<nb::Valor, nl::base::Erro> valorDoTexto(const QString& texto, const std::string& caminho)
{
    const auto valor = nb::Valor::deTextoPtBr(texto);
    if (!valor) {
        return std::unexpected(nl::base::Erro{valor.error().codigo(), caminho, valor.error().mensagem()});
    }
    return *valor;
}

} // namespace

std::expected<nb::Valor, nl::base::Erro> lerValorObrigatorio(const QJsonObject& pai, std::string_view caminhoPai, std::string_view chave)
{
    const auto texto = campos::textoObrigatorio(pai, caminhoPai, chave);
    if (!texto) {
        return std::unexpected(texto.error());
    }
    return valorDoTexto(*texto, campos::caminhoDe(caminhoPai, chave));
}

} // namespace nl::mid::composicao::leitores
