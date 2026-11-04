// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// HKDF-SHA256 (NL-3.2b, RFC 5869) pela API EVP_KDF do OpenSSL 3. Salt e info vazios não entram como parâmetro.

#include <nl/foundation/cripto/hkdf.h>

#include "nl/foundation/cripto/erros.h"

#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>

#include <array>
#include <cstddef>
#include <memory>

namespace nl::foundation::cripto {

namespace {

// RFC 5869, seção 2.3: o expand com SHA-256 produz no máximo 255 blocos de 32 bytes.
constexpr std::size_t kSaidaMaximaHkdfSha256 = 255 * 32;

} // namespace

std::expected<void, nl::base::Erro> hkdfSha256(QByteArrayView chave, QByteArrayView salt, QByteArrayView info,
                                               std::span<unsigned char> saida)
{
    using Kdf = std::unique_ptr<EVP_KDF, decltype(&EVP_KDF_free)>;
    using Contexto = std::unique_ptr<EVP_KDF_CTX, decltype(&EVP_KDF_CTX_free)>;

    if (saida.empty() || saida.size() > kSaidaMaximaHkdfSha256) {
        return std::unexpected(detalhe::erroCripto("cripto.tamanho", "saída do HKDF fora de 1 a 8160 bytes"));
    }

    static const unsigned char vazio = 0;
    const Kdf kdf{EVP_KDF_fetch(nullptr, "HKDF", nullptr), &EVP_KDF_free};
    if (!kdf) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não tem HKDF"));
    }
    const Contexto contexto{EVP_KDF_CTX_new(kdf.get()), &EVP_KDF_CTX_free};
    if (!contexto) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não criou o contexto do HKDF"));
    }

    char digest[] = "SHA256";
    const auto* chaveBytes = chave.isEmpty() ? &vazio : reinterpret_cast<const unsigned char*>(chave.data());
    const auto* saltBytes = salt.isEmpty() ? &vazio : reinterpret_cast<const unsigned char*>(salt.data());
    const auto* infoBytes = info.isEmpty() ? &vazio : reinterpret_cast<const unsigned char*>(info.data());
    std::array<OSSL_PARAM, 5> parametros{};
    std::size_t posicao = 0;
    parametros[posicao++] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, digest, sizeof(digest) - 1);
    parametros[posicao++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, const_cast<unsigned char*>(chaveBytes),
                                                              static_cast<std::size_t>(chave.size()));
    if (!salt.isEmpty()) {
        parametros[posicao++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                                                  const_cast<unsigned char*>(saltBytes),
                                                                  static_cast<std::size_t>(salt.size()));
    }
    if (!info.isEmpty()) {
        parametros[posicao++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO,
                                                                  const_cast<unsigned char*>(infoBytes),
                                                                  static_cast<std::size_t>(info.size()));
    }
    parametros[posicao] = OSSL_PARAM_construct_end();

    if (EVP_KDF_derive(contexto.get(), saida.data(), saida.size(), parametros.data()) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL falhou ao derivar com HKDF"));
    }
    return {};
}

} // namespace nl::foundation::cripto
