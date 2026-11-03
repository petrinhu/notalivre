// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Contexto de biblioteca do OpenSSL para decifrar o .pfx (NL-2.2, decisão do CISO, item 6). Interno ao módulo.
// O legacy nunca entra no contexto global: vive só no OSSL_LIB_CTX criado aqui, e some quando o contexto é liberado.
#ifndef NL_FOUNDATION_PKCS12_CONTEXTO_OSSL_H
#define NL_FOUNDATION_PKCS12_CONTEXTO_OSSL_H

#include <expected>

struct ossl_lib_ctx_st;

namespace nl::foundation::pkcs12 {

enum class FalhaContexto {
    Criacao,            ///< OSSL_LIB_CTX_new ou o provider default falharam.
    LegadoIndisponivel, ///< O provider legacy não carregou (módulo ausente ou desligado).
};

/// Move-only. Libera o OSSL_LIB_CTX no destrutor. Só usar depois de as partes extraídas dele terem saído.
class ContextoOssl {
public:
    /// Contexto com o provider default; com comLegado, também o legacy (só na segunda tentativa).
    [[nodiscard]] static std::expected<ContextoOssl, FalhaContexto> criar(bool comLegado);

    ContextoOssl(ContextoOssl&& outra) noexcept;
    ContextoOssl& operator=(ContextoOssl&& outra) noexcept;
    ContextoOssl(const ContextoOssl&) = delete;
    ContextoOssl& operator=(const ContextoOssl&) = delete;
    ~ContextoOssl();

    [[nodiscard]] ossl_lib_ctx_st* get() const noexcept { return ctx_; }

private:
    explicit ContextoOssl(ossl_lib_ctx_st* ctx) noexcept;
    ossl_lib_ctx_st* ctx_;
};

/// Põe o contexto como default desta thread enquanto viver e restaura o anterior ao sair (RAII).
/// Thread-local no OpenSSL 3 (medido na fonte, crypto/context.c): nenhum outro thread vê o contexto.
class AmbienteDefault {
public:
    explicit AmbienteDefault(ossl_lib_ctx_st* ctx) noexcept;
    ~AmbienteDefault();
    AmbienteDefault(const AmbienteDefault&) = delete;
    AmbienteDefault& operator=(const AmbienteDefault&) = delete;

    /// Falso se o OpenSSL não conseguiu trocar o default (nesse caso nada foi alterado).
    [[nodiscard]] bool ativo() const noexcept { return anterior_ != nullptr; }

private:
    ossl_lib_ctx_st* anterior_;
};

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_CONTEXTO_OSSL_H
