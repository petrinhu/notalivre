// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Verificação de cadeia (NL-2.2). Âncoras e intermediárias não são donas dos objetos: só emprestam ponteiros.
#include "verificador_cadeia.h"

#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>

#include <memory>

namespace nl::foundation::pkcs12 {

namespace {

struct LiberadorLoja {
    void operator()(X509_STORE* loja) const noexcept { X509_STORE_free(loja); }
};
struct LiberadorContextoVerificacao {
    void operator()(X509_STORE_CTX* ctx) const noexcept { X509_STORE_CTX_free(ctx); }
};
struct LiberadorPilha {
    void operator()(STACK_OF(X509) * pilha) const noexcept { sk_X509_free(pilha); }
};

} // namespace

std::expected<void, ErroCredencial> verificarCadeia(x509_st* folha,
                                                    const std::vector<CertificadoX509>& intermediarios,
                                                    const Ancoras& ancoras) {
    std::unique_ptr<X509_STORE, LiberadorLoja> loja(X509_STORE_new());
    if (!loja) {
        ERR_clear_error();
        return std::unexpected(ErroCredencial::FalhaInterna);
    }
    for (x509_st* ancora : ancoras.certificados()) {
        if (X509_STORE_add_cert(loja.get(), ancora) != 1) {
            ERR_clear_error();
            return std::unexpected(ErroCredencial::FalhaInterna);
        }
    }
    // O tempo é checado antes, com Vencido. Aqui só a cadeia: assim o vencido não vira CadeiaInvalida.
    X509_STORE_set_flags(loja.get(), X509_V_FLAG_NO_CHECK_TIME);

    std::unique_ptr<STACK_OF(X509), LiberadorPilha> pilha(sk_X509_new_null());
    if (!pilha) {
        ERR_clear_error();
        return std::unexpected(ErroCredencial::FalhaInterna);
    }
    for (const CertificadoX509& intermediario : intermediarios) {
        if (sk_X509_push(pilha.get(), intermediario.get()) == 0) {
            ERR_clear_error();
            return std::unexpected(ErroCredencial::FalhaInterna);
        }
    }

    std::unique_ptr<X509_STORE_CTX, LiberadorContextoVerificacao> contexto(X509_STORE_CTX_new());
    if (!contexto || X509_STORE_CTX_init(contexto.get(), loja.get(), folha, pilha.get()) != 1) {
        ERR_clear_error();
        return std::unexpected(ErroCredencial::FalhaInterna);
    }
    const int valida = X509_verify_cert(contexto.get());
    ERR_clear_error();
    if (valida != 1) {
        return std::unexpected(ErroCredencial::CadeiaInvalida);
    }
    return {};
}

} // namespace nl::foundation::pkcs12
