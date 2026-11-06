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
    contexto.provedorDefault_ = OSSL_PROVIDER_load(ctx, "default");
    if (contexto.provedorDefault_ == nullptr) {
        ERR_clear_error();
        return std::unexpected(FalhaContexto::Criacao);
    }
    if (comLegado) {
        contexto.provedorLegado_ = OSSL_PROVIDER_load(ctx, "legacy");
        if (contexto.provedorLegado_ == nullptr) {
            ERR_clear_error();
            return std::unexpected(FalhaContexto::LegadoIndisponivel);
        }
    }
    return contexto;
}

ContextoOssl::ContextoOssl(OSSL_LIB_CTX* ctx) noexcept
    : ctx_(ctx) {}

ContextoOssl::ContextoOssl(ContextoOssl&& outra) noexcept
    : ctx_(std::exchange(outra.ctx_, nullptr)),
      provedorDefault_(std::exchange(outra.provedorDefault_, nullptr)),
      provedorLegado_(std::exchange(outra.provedorLegado_, nullptr)) {}

ContextoOssl& ContextoOssl::operator=(ContextoOssl&& outra) noexcept {
    if (this != &outra) {
        liberar();
        ctx_ = std::exchange(outra.ctx_, nullptr);
        provedorDefault_ = std::exchange(outra.provedorDefault_, nullptr);
        provedorLegado_ = std::exchange(outra.provedorLegado_, nullptr);
    }
    return *this;
}

ContextoOssl::~ContextoOssl() {
    liberar();
}

void ContextoOssl::liberar() noexcept {
    // Ordem: os providers saem antes do contexto. Sem o unload, o provider default sobra vivo no processo (medido).
    if (provedorLegado_ != nullptr) {
        OSSL_PROVIDER_unload(provedorLegado_);
    }
    if (provedorDefault_ != nullptr) {
        OSSL_PROVIDER_unload(provedorDefault_);
    }
    OSSL_LIB_CTX_free(ctx_);
    ctx_ = nullptr;
    provedorDefault_ = nullptr;
    provedorLegado_ = nullptr;
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
