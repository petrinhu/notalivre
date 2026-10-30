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
// Chave de acesso de 50 posições (NL-2.8, DA-2.21, ADR-0005 b): 9 dígitos, 14 caracteres [0-9A-Z] (a inscrição, posições
// 10 a 23) e 27 dígitos. O Id da NFS-e de 53 é "NFS" mais a chave: a inscrição fica nas posições 13 a 26. INFERÊNCIA do
// leiaute pela ADR-0005. Chave fora desse leiaute não é mascarada: nullopt, e o chamador descarta.
constexpr std::size_t kTamanhoChaveAcesso = 50;
constexpr std::size_t kPosicaoInscricaoChave = 9;      // índice 0 da posição 10 (1-based)
constexpr std::size_t kTamanhoIdNfse = 53;
constexpr std::size_t kPosicaoInscricaoIdNfse = 12;    // índice 0 da posição 13 (1-based)
constexpr std::string_view kPrefixoIdNfse = "NFS";

bool chaveNoLeiaute(std::string_view chave)
{
    if (chave.size() != kTamanhoChaveAcesso) {
        return false;
    }
    for (std::size_t i = 0; i < kTamanhoChaveAcesso; ++i) {
        const char c = chave[i];
        const bool digito = c >= '0' && c <= '9';
        const bool alfanumerico = digito || (c >= 'A' && c <= 'Z');
        const bool inscricao = i >= kPosicaoInscricaoChave && i < kPosicaoInscricaoChave + kTamanhoInscricao;
        if (inscricao ? !alfanumerico : !digito) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> mascararInscricao(std::string_view valor)
{
    if (valor.size() == kTamanhoIdDps && valor.starts_with(kPrefixoIdDps)) {
        std::string mascarado{valor};
        mascarado.replace(kPosicaoInscricao, kTamanhoInscricao, kTamanhoInscricao, '#');
        return mascarado;
    }
    if (valor.size() == kTamanhoChaveAcesso && chaveNoLeiaute(valor)) {
        std::string mascarado{valor};
        mascarado.replace(kPosicaoInscricaoChave, kTamanhoInscricao, kTamanhoInscricao, '#');
        return mascarado;
    }
    if (valor.size() == kTamanhoIdNfse && valor.starts_with(kPrefixoIdNfse) &&
        chaveNoLeiaute(valor.substr(kPrefixoIdNfse.size()))) {
        std::string mascarado{valor};
        mascarado.replace(kPosicaoInscricaoIdNfse, kTamanhoInscricao, kTamanhoInscricao, '#');
        return mascarado;
    }
    return std::nullopt;
}

// Lista da ADR-0005 §1 (30 nomes da NL-2.7) mais os 14 da DA-2.23 (decisão do CISO, 28/10/2026). Comparação sem caixa.
// A mesma lista está no teste campoProibidoNuncaSaiMesmoDeclarado.
constexpr auto kCamposProibidos = std::to_array<std::string_view>({
    "senhaMestra",           "senhaCertificado",      "senhaSmtp",           "tokenOauth",
    "chaveDek",              "chaveKek",              "codigoRecuperacao",   "conteudoA1",
    "cpfTomador",            "cnpjTomador",           "nome",                "endereco",
    "telefone",              "email",                 "inscricaoMunicipal",  "corpoEmail",
    "anexoEmail",            "xmlDps",                "xmlNfse",             "xmlEvento",
    "corpoHttp",             "textoErroApi",          "cabecalhoAutenticacao", "cookie",
    "dadosCertificado",      "nomeArquivo",           "caminhoArquivo",      "senha",
    "token",                 "envelopeAssinado",
    "cpfEmitente",           "cnpjEmitente",          "inscricaoEmitente",   "cpf",
    "cnpj",                  "inscricao",             "nifTomador",          "titularCertificado",
    "numeroSerieCertificado", "chavePrivada",         "chaveHeadless",       "senhaBanco",
    "corpoRequisicao",       "corpoResposta",
});

// Radicais do lado fechado: nome que contém um deles, sem caixa, é proibido. Cobre variações como "Senha" e
// "senhaSmtpServidor". O custo é descartar um campo a mais; nunca vazar um a menos. Sem "serie", "numero", "tipo" nem
// "codigo", que a própria NFS-e precisa para suporte.
constexpr auto kRadicaisProibidos = std::to_array<std::string_view>({
    "senha", "token", "chave", "cpf", "cnpj", "nif", "titular", "certificado", "numeroserie", "xml", "corpo",
    "conteudo", "cookie", "nome", "endereco", "telefone", "email", "inscricao", "cabecalho", "caminho", "anexo",
    "textoerro", "recupera", "envelope",
});

constexpr auto kCamposSoMascara = std::to_array<std::string_view>({"idDps", "idNfse", "chaveAcesso"});

std::string minusculoAscii(std::string_view texto)
{
    std::string saida{texto};
    for (char& caractere : saida) {
        if (caractere >= 'A' && caractere <= 'Z') {
            caractere = static_cast<char>(caractere - 'A' + 'a');
        }
    }
    return saida;
}

bool somenteAscii(std::string_view texto)
{
    return std::ranges::all_of(texto, [](char caractere) { return static_cast<unsigned char>(caractere) < 0x80; });
}

bool igualSemCaixa(std::string_view esquerdo, std::string_view direito)
{
    return minusculoAscii(esquerdo) == minusculoAscii(direito);
}

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

bool ehCampoProibido(std::string_view nome)
{
    if (nome.empty() || !somenteAscii(nome)) {
        return true;  // falha fechada: nome vazio ou fora do ASCII nunca sai
    }
    const std::string minusculo = minusculoAscii(nome);
    const bool naLista = std::ranges::any_of(kCamposProibidos, [&minusculo](std::string_view proibido) {
        return igualSemCaixa(proibido, minusculo);
    });
    if (naLista) {
        return true;
    }
    return std::ranges::any_of(kRadicaisProibidos, [&minusculo](std::string_view radical) {
        return minusculo.find(radical) != std::string::npos;
    });
}

bool ehCampoSoMascara(std::string_view nome)
{
    return std::ranges::any_of(kCamposSoMascara, [nome](std::string_view campo) { return igualSemCaixa(campo, nome); });
}

std::span<const std::string_view> radicaisDeCampoProibido() noexcept
{
    return kRadicaisProibidos;
}

std::span<const std::string_view> camposProibidosDeReferencia() noexcept
{
    return kCamposProibidos;
}

} // namespace nl::foundation::log
