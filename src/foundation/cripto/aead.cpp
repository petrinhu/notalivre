// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// AES-256-GCM (NL-3.2b, ADR-0003 regra 1) sobre OpenSSL 3 (EVP). O tipo do OpenSSL fica só neste .cpp.

#include <nl/foundation/cripto/aead.h>

#include "nl/foundation/cripto/erros.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>

namespace nl::foundation::cripto {

namespace {

using ContextoCifra = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

ContextoCifra novoContexto()
{
    return ContextoCifra{EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free};
}

// Ponteiro válido mesmo para sequência vazia, porque a biblioteca não aceita nulo com tamanho zero em alguns caminhos.
const unsigned char* ponteiroDe(QByteArrayView dado, const unsigned char& vazio)
{
    return dado.isEmpty() ? &vazio : reinterpret_cast<const unsigned char*>(dado.data());
}

bool cabeEmInt(std::size_t tamanho)
{
    return tamanho <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

} // namespace

std::expected<Cifrado, nl::base::Erro> cifrarAes256Gcm(QByteArrayView chave, QByteArrayView nonce, QByteArrayView aad,
                                                       QByteArrayView claro)
{
    if (chave.size() != kTamanhoChave) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "a chave AES-256 precisa ter 32 bytes"));
    }
    if (nonce.size() != kTamanhoNonce) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "o nonce do AES-GCM precisa ter 12 bytes"));
    }
    if (!cabeEmInt(static_cast<std::size_t>(claro.size())) || !cabeEmInt(static_cast<std::size_t>(aad.size()))) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "entrada grande demais para o AES-GCM"));
    }

    static const unsigned char vazio = 0;
    const auto contexto = novoContexto();
    if (!contexto || EVP_EncryptInit_ex(contexto.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não iniciou o AES-GCM"));
    }
    if (EVP_EncryptInit_ex(contexto.get(), nullptr, nullptr, ponteiroDe(chave, vazio), ponteiroDe(nonce, vazio)) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL recusou chave ou nonce"));
    }

    // A AAD não produz saída: o contador dela é próprio, para não entrar no tamanho do claro.
    int ignorado = 0;
    if (!aad.isEmpty() && EVP_EncryptUpdate(contexto.get(), nullptr, &ignorado, ponteiroDe(aad, vazio),
                                             static_cast<int>(aad.size())) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL recusou a AAD"));
    }

    QByteArray textoComTag(static_cast<qsizetype>(claro.size()) + kTamanhoTag, Qt::Uninitialized);
    auto* cifra = reinterpret_cast<unsigned char*>(textoComTag.data());
    int escritos = 0;
    if (!claro.isEmpty() && EVP_EncryptUpdate(contexto.get(), cifra, &escritos, ponteiroDe(claro, vazio),
                                               static_cast<int>(claro.size())) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL falhou ao cifrar"));
    }
    int finalizados = 0;
    if (EVP_EncryptFinal_ex(contexto.get(), cifra + escritos, &finalizados) != 1 || finalizados != 0) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL falhou ao fechar a cifragem"));
    }

    std::array<unsigned char, kTamanhoTag> tag{};
    if (EVP_CIPHER_CTX_ctrl(contexto.get(), EVP_CTRL_GCM_GET_TAG, kTamanhoTag, tag.data()) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não devolveu a tag"));
    }
    std::copy(tag.begin(), tag.end(), cifra + escritos);

    return Cifrado{QByteArray(nonce.data(), nonce.size()), std::move(textoComTag)};
}

std::expected<int, nl::base::Erro> decifrarAes256Gcm(QByteArrayView chave, QByteArrayView nonce, QByteArrayView aad,
                                                     QByteArrayView textoComTag, std::span<unsigned char> claro)
{
    // Em qualquer retorno de erro, a área de saída volta zerada: o claro pode ter sido escrito antes da tag ser conferida.
    const auto falha = [&claro](const char* codigo, const char* mensagem) {
        OPENSSL_cleanse(claro.data(), claro.size());
        return std::unexpected(detalhe::erroCripto(codigo, mensagem));
    };

    if (chave.size() != kTamanhoChave) {
        return falha("cripto.tamanho", "a chave AES-256 precisa ter 32 bytes");
    }
    if (nonce.size() != kTamanhoNonce) {
        return falha("cripto.tamanho", "o nonce do AES-GCM precisa ter 12 bytes");
    }
    if (textoComTag.size() < kTamanhoTag) {
        return falha("cripto.tamanho", "texto cifrado menor que a tag de 16 bytes");
    }
    const auto tamanhoCifra = static_cast<std::size_t>(textoComTag.size() - kTamanhoTag);
    if (claro.size() < tamanhoCifra) {
        return falha("cripto.tamanho", "área de saída menor que o texto cifrado");
    }
    if (!cabeEmInt(tamanhoCifra) || !cabeEmInt(static_cast<std::size_t>(aad.size()))) {
        return falha("cripto.tamanho", "entrada grande demais para o AES-GCM");
    }

    static const unsigned char vazio = 0;
    const auto* cifra = reinterpret_cast<const unsigned char*>(textoComTag.data());
    std::array<unsigned char, kTamanhoTag> tag{};
    std::copy(cifra + tamanhoCifra, cifra + tamanhoCifra + kTamanhoTag, tag.begin());

    const auto contexto = novoContexto();
    if (!contexto || EVP_DecryptInit_ex(contexto.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return falha("cripto.biblioteca", "o OpenSSL não iniciou o AES-GCM");
    }
    if (EVP_DecryptInit_ex(contexto.get(), nullptr, nullptr, ponteiroDe(chave, vazio), ponteiroDe(nonce, vazio)) != 1) {
        return falha("cripto.biblioteca", "o OpenSSL recusou chave ou nonce");
    }

    // Mesmo cuidado de cifrarAes256Gcm: a AAD tem contador próprio, e o do claro começa em zero.
    int ignorado = 0;
    if (!aad.isEmpty() && EVP_DecryptUpdate(contexto.get(), nullptr, &ignorado, ponteiroDe(aad, vazio),
                                             static_cast<int>(aad.size())) != 1) {
        return falha("cripto.biblioteca", "o OpenSSL recusou a AAD");
    }
    int escritos = 0;
    if (tamanhoCifra > 0 && EVP_DecryptUpdate(contexto.get(), claro.data(), &escritos, cifra,
                                               static_cast<int>(tamanhoCifra)) != 1) {
        return falha("cripto.biblioteca", "o OpenSSL falhou ao decifrar");
    }
    if (EVP_CIPHER_CTX_ctrl(contexto.get(), EVP_CTRL_GCM_SET_TAG, kTamanhoTag, tag.data()) != 1) {
        return falha("cripto.biblioteca", "o OpenSSL recusou a tag");
    }
    int finalizados = 0;
    if (EVP_DecryptFinal_ex(contexto.get(), claro.data() + escritos, &finalizados) != 1) {
        return falha("cripto.tag", "autenticação do AES-GCM falhou: dado, AAD, nonce ou chave diferentes");
    }
    return escritos + finalizados;
}

} // namespace nl::foundation::cripto
