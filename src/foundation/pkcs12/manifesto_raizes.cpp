// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do SHA256SUMS das raízes da ICP-Brasil (NL-2.13, DA-2.47). Puro: o texto entra, nenhum arquivo é aberto aqui.
// Formato: "<sha256 de 64 hex minúsculas>  <arquivo>" e, por arquivo, "# meta <arquivo> proposito=... fonte=...".
// Qualquer linha fora desse formato, que não seja comentário ou vazia, reprova o manifesto (fail-closed).
#include <nl/foundation/pkcs12/manifesto_raizes.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nl::foundation::pkcs12 {

namespace {

constexpr std::string_view PREFIXO_META = "# meta ";
constexpr std::size_t TAMANHO_SHA256_HEX = 64;

struct LinhaMeta {
    std::string arquivo;
    std::string proposito;
    std::string fonte;
};

struct LinhaHash {
    std::string sha256;
    std::string arquivo;
};

bool ehSha256Hex(std::string_view texto) {
    if (texto.size() != TAMANHO_SHA256_HEX) {
        return false;
    }
    for (const char c : texto) {
        const bool digito = c >= '0' && c <= '9';
        const bool minuscula = c >= 'a' && c <= 'f';
        if (!digito && !minuscula) {
            return false;
        }
    }
    return true;
}

// Nome sem separador de caminho: só letras, dígitos, '_', '.' e '-'. "." e ".." ficam de fora.
bool ehNomeDeArquivo(std::string_view nome) {
    if (nome.empty() || nome == "." || nome == "..") {
        return false;
    }
    for (const char c : nome) {
        const bool letra = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool digito = c >= '0' && c <= '9';
        const bool permitido = letra || digito || c == '_' || c == '.' || c == '-';
        if (!permitido) {
            return false;
        }
    }
    return true;
}

std::vector<std::string_view> separarTokens(std::string_view linha) {
    std::vector<std::string_view> tokens;
    std::size_t inicio = 0;
    while (inicio < linha.size()) {
        const std::size_t fim = linha.find(' ', inicio);
        const std::size_t tamanho = (fim == std::string_view::npos ? linha.size() : fim) - inicio;
        if (tamanho > 0) {
            tokens.push_back(linha.substr(inicio, tamanho));
        }
        if (fim == std::string_view::npos) {
            break;
        }
        inicio = fim + 1;
    }
    return tokens;
}

// "# meta <arquivo> chave=valor ..." . Chaves desconhecidas são ignoradas; proposito e fonte são obrigatórias.
std::optional<LinhaMeta> lerLinhaMeta(std::string_view linha) {
    const std::vector<std::string_view> tokens = separarTokens(linha.substr(PREFIXO_META.size()));
    if (tokens.empty() || !ehNomeDeArquivo(tokens.front())) {
        return std::nullopt;
    }
    LinhaMeta meta;
    meta.arquivo = std::string(tokens.front());
    bool temProposito = false;
    bool temFonte = false;
    for (std::size_t i = 1; i < tokens.size(); ++i) {
        const std::size_t igual = tokens[i].find('=');
        if (igual == std::string_view::npos || igual == 0) {
            return std::nullopt;
        }
        const std::string_view chave = tokens[i].substr(0, igual);
        const std::string_view valor = tokens[i].substr(igual + 1);
        if (chave == "proposito") {
            meta.proposito = std::string(valor);
            temProposito = true;
        } else if (chave == "fonte") {
            meta.fonte = std::string(valor);
            temFonte = true;
        }
    }
    if (!temProposito || !temFonte || meta.proposito.empty() || meta.fonte.empty()) {
        return std::nullopt;
    }
    return meta;
}

// "<sha256>  <arquivo>": exatamente dois espaços entre o hash e o nome.
std::optional<LinhaHash> lerLinhaHash(std::string_view linha) {
    if (linha.size() <= TAMANHO_SHA256_HEX + 2 || linha.substr(TAMANHO_SHA256_HEX, 2) != "  ") {
        return std::nullopt;
    }
    const std::string_view sha = linha.substr(0, TAMANHO_SHA256_HEX);
    const std::string_view nome = linha.substr(TAMANHO_SHA256_HEX + 2);
    if (!ehSha256Hex(sha) || !ehNomeDeArquivo(nome)) {
        return std::nullopt;
    }
    return LinhaHash{std::string(sha), std::string(nome)};
}

bool contemArquivo(const std::vector<LinhaHash>& linhas, const std::string& arquivo) {
    for (const LinhaHash& linha : linhas) {
        if (linha.arquivo == arquivo) {
            return true;
        }
    }
    return false;
}

bool contemMeta(const std::vector<LinhaMeta>& metas, const std::string& arquivo) {
    for (const LinhaMeta& meta : metas) {
        if (meta.arquivo == arquivo) {
            return true;
        }
    }
    return false;
}

} // namespace

std::expected<std::vector<EntradaRaiz>, ErroRaizIcp> lerManifesto(QByteArrayView texto) {
    const std::string conteudo(texto.data(), static_cast<std::size_t>(texto.size()));
    std::vector<LinhaHash> hashes;
    std::vector<LinhaMeta> metas;

    std::size_t inicio = 0;
    while (inicio <= conteudo.size()) {
        std::size_t fim = conteudo.find('\n', inicio);
        if (fim == std::string::npos) {
            fim = conteudo.size();
        }
        std::string_view linha(conteudo.data() + inicio, fim - inicio);
        if (!linha.empty() && linha.back() == '\r') {
            linha.remove_suffix(1);
        }
        inicio = fim + 1;

        if (linha.empty()) {
            continue;
        }
        if (linha.substr(0, PREFIXO_META.size()) == PREFIXO_META) {
            std::optional<LinhaMeta> meta = lerLinhaMeta(linha);
            if (!meta || contemMeta(metas, meta->arquivo)) {
                return std::unexpected(ErroRaizIcp::ManifestoMalformado);
            }
            metas.push_back(std::move(*meta));
            continue;
        }
        if (linha.front() == '#') {
            continue;
        }
        std::optional<LinhaHash> hash = lerLinhaHash(linha);
        if (!hash || contemArquivo(hashes, hash->arquivo)) {
            return std::unexpected(ErroRaizIcp::ManifestoMalformado);
        }
        hashes.push_back(std::move(*hash));
    }

    if (hashes.empty()) {
        return std::unexpected(ErroRaizIcp::ManifestoVazio);
    }
    std::vector<EntradaRaiz> entradas;
    entradas.reserve(hashes.size());
    for (const LinhaHash& hash : hashes) {
        const LinhaMeta* correspondente = nullptr;
        for (const LinhaMeta& meta : metas) {
            if (meta.arquivo == hash.arquivo) {
                correspondente = &meta;
                break;
            }
        }
        if (correspondente == nullptr) {
            return std::unexpected(ErroRaizIcp::ManifestoMalformado);
        }
        entradas.push_back(EntradaRaiz{hash.arquivo, hash.sha256, correspondente->proposito, correspondente->fonte});
    }
    for (const LinhaMeta& meta : metas) {
        if (!contemArquivo(hashes, meta.arquivo)) {
            return std::unexpected(ErroRaizIcp::ManifestoMalformado);
        }
    }
    return entradas;
}

} // namespace nl::foundation::pkcs12
