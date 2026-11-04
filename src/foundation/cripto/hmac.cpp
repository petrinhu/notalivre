// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// HMAC-SHA256 (NL-3.2b) pela API EVP_MAC do OpenSSL 3. O tipo do OpenSSL fica só neste .cpp.

#include <nl/foundation/cripto/hmac.h>

#include "nl/foundation/cripto/erros.h"

#include <openssl/core_names.h>
#include <openssl/params.h>

#include <cstddef>
#include <memory>

namespace nl::foundation::cripto {

std::expected<void, nl::base::Erro> hmacSha256(QByteArrayView chave, QByteArrayView dado,
                                               std::span<unsigned char, 32> saida)
{
    using Mac = std::unique_ptr<EVP_MAC, decltype(&EVP_MAC_free)>;
    using Contexto = std::unique_ptr<EVP_MAC_CTX, decltype(&EVP_MAC_CTX_free)>;

    static const unsigned char vazio = 0;
    const Mac mac{EVP_MAC_fetch(nullptr, "HMAC", nullptr), &EVP_MAC_free};
    if (!mac) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não tem HMAC"));
    }
    const Contexto contexto{EVP_MAC_CTX_new(mac.get()), &EVP_MAC_CTX_free};
    if (!contexto) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não criou o contexto do HMAC"));
    }

    char digest[] = "SHA256";
    const OSSL_PARAM parametros[] = {
        OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest, sizeof(digest) - 1),
        OSSL_PARAM_construct_end(),
    };
    const auto* chaveBytes = chave.isEmpty() ? &vazio : reinterpret_cast<const unsigned char*>(chave.data());
    if (EVP_MAC_init(contexto.get(), chaveBytes, static_cast<std::size_t>(chave.size()), parametros) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL recusou a chave do HMAC"));
    }
    if (!dado.isEmpty() && EVP_MAC_update(contexto.get(), reinterpret_cast<const unsigned char*>(dado.data()),
                                          static_cast<std::size_t>(dado.size())) != 1) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL falhou ao processar o dado do HMAC"));
    }
    std::size_t escritos = 0;
    if (EVP_MAC_final(contexto.get(), saida.data(), &escritos, saida.size()) != 1 || escritos != saida.size()) {
        return std::unexpected(detalhe::erroCripto("cripto.biblioteca", "o OpenSSL não fechou o HMAC"));
    }
    return {};
}

} // namespace nl::foundation::cripto
