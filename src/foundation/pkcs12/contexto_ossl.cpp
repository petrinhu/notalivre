// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Contexto próprio e ambiente default por thread (NL-2.2). Ver contexto_ossl.h.
#include "contexto_ossl.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/provider.h>

#include <utility>

namespace nl::foundation::pkcs12 {

std::expected<ContextoOssl, FalhaContexto> ContextoOssl::criar(bool comLegado) {
    OSSL_LIB_CTX* ctx = OSSL_LIB_CTX_new();
    if (ctx == nullptr) {
        ERR_clear_error();
        return std::unexpected(FalhaContexto::Criacao);
    }
    // Dono desde já: qualquer retorno de erro abaixo libera o contexto pelo destrutor.
    ContextoOssl contexto(ctx);
    // Carregar só o legacy desliga o default (o OpenSSL ativa o default sozinho apenas quando nenhum provider foi
    // carregado explicitamente). Por isso o default entra sempre, explícito.
    if (OSSL_PROVIDER_load(ctx, "default") == nullptr) {
        ERR_clear_error();
        return std::unexpected(FalhaContexto::Criacao);
    }
    if (comLegado && OSSL_PROVIDER_load(ctx, "legacy") == nullptr) {
        ERR_clear_error();
        return std::unexpected(FalhaContexto::LegadoIndisponivel);
    }
    return contexto;
}

ContextoOssl::ContextoOssl(OSSL_LIB_CTX* ctx) noexcept
    : ctx_(ctx) {}

ContextoOssl::ContextoOssl(ContextoOssl&& outra) noexcept
    : ctx_(std::exchange(outra.ctx_, nullptr)) {}

ContextoOssl& ContextoOssl::operator=(ContextoOssl&& outra) noexcept {
    if (this != &outra) {
        OSSL_LIB_CTX_free(ctx_);
        ctx_ = std::exchange(outra.ctx_, nullptr);
    }
    return *this;
}

ContextoOssl::~ContextoOssl() {
    OSSL_LIB_CTX_free(ctx_);
}

AmbienteDefault::AmbienteDefault(OSSL_LIB_CTX* ctx) noexcept
    : anterior_(OSSL_LIB_CTX_set0_default(ctx)) {}

AmbienteDefault::~AmbienteDefault() {
    // set0_default devolve o default anterior; se ele não estava ativo, não houve troca e não há o que restaurar.
    if (anterior_ != nullptr) {
        OSSL_LIB_CTX_set0_default(anterior_);
    }
}

} // namespace nl::foundation::pkcs12
