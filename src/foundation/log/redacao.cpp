// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Regras de redação do Log (NL-2.7, ADR-S4). Função pura, sem I/O.

#include <nl/foundation/log/redacao.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace nl::foundation::log {
namespace {

// Id da DPS com 45 posições: DPS (1 a 3), município (4 a 10), tipo (11), inscrição (12 a 25), série (26 a 30),
// número (31 a 45). A inscrição de CPF e de CNPJ ocupa as mesmas 14 posições, por isso a regra vale para os dois.
constexpr std::size_t kTamanhoIdDps = 45;
constexpr std::size_t kPosicaoInscricao = 11;  // índice 0 da posição 12 (1-based)
constexpr std::size_t kTamanhoInscricao = 14;
constexpr std::string_view kPrefixoIdDps = "DPS";

/// Troca as 14 posições da inscrição por '#'. Valor de outro tamanho ou sem o prefixo DPS não é mascarado: nullopt,
/// e o chamador descarta (falha fechada: nunca grava um Id que pode conter a inscrição sem máscara).
std::optional<std::string> mascararInscricao(std::string_view valor)
{
    if (valor.size() != kTamanhoIdDps || !valor.starts_with(kPrefixoIdDps)) {
        return std::nullopt;
    }
    std::string mascarado{valor};
    mascarado.replace(kPosicaoInscricao, kTamanhoInscricao, kTamanhoInscricao, '#');
    return mascarado;
}

// Lista da ADR-0005 §1, classe Proibido, em nomes de campo. A mesma lista está no teste campoProibidoNuncaSaiMesmoDeclarado.
constexpr std::array<std::string_view, 30> kCamposProibidos = {
    "senhaMestra",           "senhaCertificado",      "senhaSmtp",       "tokenOauth",       "chaveDek",
    "chaveKek",              "codigoRecuperacao",     "conteudoA1",      "cpfTomador",       "cnpjTomador",
    "nome",                  "endereco",              "telefone",        "email",            "inscricaoMunicipal",
    "corpoEmail",            "anexoEmail",            "xmlDps",          "xmlNfse",          "xmlEvento",
    "corpoHttp",             "textoErroApi",          "cabecalhoAutenticacao", "cookie",     "dadosCertificado",
    "nomeArquivo",           "caminhoArquivo",        "senha",           "token",            "envelopeAssinado",
};

} // namespace

std::optional<std::string> aplicarRegra(Regra regra, std::string_view valor)
{
    switch (regra) {
    case Regra::Cru:
        return std::string{valor};
    case Regra::InscricaoMascarada:
        return mascararInscricao(valor);
    case Regra::Proibido:
        return std::nullopt;
    }
    return std::nullopt;  // falha fechada
}

bool ehCampoProibido(std::string_view nome) noexcept
{
    return std::ranges::find(kCamposProibidos, nome) != kCamposProibidos.end();
}

} // namespace nl::foundation::log
