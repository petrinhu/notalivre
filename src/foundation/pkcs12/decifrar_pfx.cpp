// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Decifra o .pfx com PKCS12_parse dentro de um OSSL_LIB_CTX próprio, como default da thread (NL-2.2, decisão do
// CISO, item 6). Caminho único para todas as versões do OpenSSL: a chave e os certificados saem para o contexto
// default por DER (passado por OPENSSL_cleanse) antes de o contexto próprio ser liberado.
#include "decifrar_pfx.h"

#include <nl/foundation/pkcs12/classificacao_erros.h>
#include "contexto_ossl.h"

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
#include <openssl/pkcs12err.h>
#include <openssl/pkcs7.h>
#include <openssl/x509.h>

#include <climits>
#include <cstring>
#include <cstddef>
#include <utility>

namespace nl::foundation::pkcs12 {

namespace {

using interno::Falha;


/// Chave privada fora de bag cifrado (keyBag em data safe): proibida pela ADR-0002. Lê só o authsafes, sem senha.
/// Falha fechada: se o authsafes não desempacota, ou um data safe não desempacota, a chave conta como em claro.
bool possuiChaveEmClaro(PKCS12* p12) {
    STACK_OF(PKCS7)* seguros = PKCS12_unpack_authsafes(p12);
    if (seguros == nullptr) {
        ERR_clear_error();
        return true;
    }
    bool achou = false;
    for (int i = 0; i < sk_PKCS7_num(seguros) && !achou; ++i) {
        PKCS7* seguro = sk_PKCS7_value(seguros, i);
        if (OBJ_obj2nid(seguro->type) != NID_pkcs7_data) {
            continue;
        }
        STACK_OF(PKCS12_SAFEBAG)* bags = PKCS12_unpack_p7data(seguro);
        if (bags == nullptr) {
            ERR_clear_error();
            achou = true;
            break;
        }
        for (int j = 0; j < sk_PKCS12_SAFEBAG_num(bags) && !achou; ++j) {
            achou = PKCS12_SAFEBAG_get_nid(sk_PKCS12_SAFEBAG_value(bags, j)) == NID_keyBag;
        }
        sk_PKCS12_SAFEBAG_pop_free(bags, PKCS12_SAFEBAG_free);
    }
    sk_PKCS7_pop_free(seguros, PKCS7_free);
    return achou;
}

/// Partes ainda ligadas ao contexto da tentativa. Declaradas DEPOIS do ContextoOssl, então são liberadas antes dele.
struct PartesDaTentativa {
    ChavePrivada chave;
    CertificadoX509 certificado;
    std::vector<CertificadoX509> intermediarios;
};


/// Exporta uma chave para DER no contexto de origem e a reimporta no default (contexto global). Zera o DER.
std::expected<ChavePrivada, Falha> reimportarChave(EVP_PKEY* origem) {
    unsigned char* der = nullptr;
    const int tamanho = i2d_PrivateKey(origem, &der);
    if (tamanho <= 0 || der == nullptr) {
        ERR_clear_error();
        return std::unexpected(Falha::Interno);
    }
    const unsigned char* leitura = der;
    EVP_PKEY* copia = d2i_AutoPrivateKey_ex(nullptr, &leitura, tamanho, nullptr, nullptr);
    OPENSSL_cleanse(der, static_cast<std::size_t>(tamanho));
    OPENSSL_free(der);
    if (copia == nullptr) {
        ERR_clear_error();
        return std::unexpected(Falha::Interno);
    }
    return ChavePrivada(copia);
}

/// Mesma coisa para um certificado (público, mas o DER passa por OPENSSL_cleanse como o resto do pipeline).
std::expected<CertificadoX509, Falha> reimportarCertificado(X509* origem) {
    unsigned char* der = nullptr;
    const int tamanho = i2d_X509(origem, &der);
    if (tamanho <= 0 || der == nullptr) {
        ERR_clear_error();
        return std::unexpected(Falha::Interno);
    }
    const unsigned char* leitura = der;
    X509* copia = d2i_X509(nullptr, &leitura, tamanho);
    OPENSSL_cleanse(der, static_cast<std::size_t>(tamanho));
    OPENSSL_free(der);
    if (copia == nullptr) {
        ERR_clear_error();
        return std::unexpected(Falha::Interno);
    }
    return CertificadoX509(copia);
}

/// Copia as partes do contexto próprio para o contexto default. Chamada fora do AmbienteDefault.
std::expected<PartesPfx, Falha> reimportar(PartesDaTentativa& brutas) {
    PartesPfx saida;
    auto chave = reimportarChave(brutas.chave.get());
    if (!chave) {
        return std::unexpected(chave.error());
    }
    saida.chave = std::move(*chave);
    auto certificado = reimportarCertificado(brutas.certificado.get());
    if (!certificado) {
        return std::unexpected(certificado.error());
    }
    saida.certificado = std::move(*certificado);
    for (const CertificadoX509& intermediario : brutas.intermediarios) {
        auto copia = reimportarCertificado(intermediario.get());
        if (!copia) {
            return std::unexpected(copia.error());
        }
        saida.intermediarios.push_back(std::move(*copia));
    }
    return saida;
}

/// Uma tentativa completa: contexto, default da thread, parse, reimportação e liberação do contexto.
std::expected<PartesPfx, Falha> tentar(QByteArrayView pfx, const SenhaSegura& senha, bool comLegado) {
    ERR_clear_error();
    if (pfx.size() <= 0 || pfx.size() > INT_MAX) {
        return std::unexpected(Falha::Malformado);
    }
    auto contexto = ContextoOssl::criar(comLegado);
    if (!contexto) {
        return std::unexpected(contexto.error() == FalhaContexto::LegadoIndisponivel ? Falha::LegadoIndisponivel
                                                                                     : Falha::Interno);
    }
    // Ordem de destruição: brutas (partes do contexto) antes de contexto. Declarado nesta ordem de propósito.
    PartesDaTentativa brutas;
    {
        AmbienteDefault ambiente(contexto->get());
        if (!ambiente.ativo()) {
            return std::unexpected(Falha::Interno);
        }
        BIO* bio = BIO_new_mem_buf(pfx.constData(), static_cast<int>(pfx.size()));
        if (bio == nullptr) {
            ERR_clear_error();
            return std::unexpected(Falha::Interno);
        }
        PKCS12* p12 = d2i_PKCS12_bio(bio, nullptr);
        BIO_free(bio);
        if (p12 == nullptr) {
            ERR_clear_error();
            return std::unexpected(Falha::Malformado);
        }
        if (possuiChaveEmClaro(p12)) {
            PKCS12_free(p12);
            return std::unexpected(Falha::Malformado);
        }
        EVP_PKEY* chave = nullptr;
        X509* certificado = nullptr;
        STACK_OF(X509)* ca = nullptr;
        const int ok = PKCS12_parse(p12, senha.comoC(), &chave, &certificado, &ca);
        PKCS12_free(p12);
        if (ok != 1) {
            const Falha motivo = interno::classificarCodigos(interno::colherErrosOpenssl());
            EVP_PKEY_free(chave);
            X509_free(certificado);
            sk_X509_pop_free(ca, X509_free);
            return std::unexpected(motivo);
        }
        if (chave == nullptr || certificado == nullptr) {
            sk_X509_pop_free(ca, X509_free);
            EVP_PKEY_free(chave);
            X509_free(certificado);
            return std::unexpected(Falha::Malformado);
        }
        brutas.chave.reset(chave);
        brutas.certificado.reset(certificado);
        while (ca != nullptr && sk_X509_num(ca) > 0) {
            // Posse antes do push: o certificado não fica sem dono se o vetor não crescer (bad_alloc).
            CertificadoX509 posse(sk_X509_pop(ca));
            brutas.intermediarios.push_back(std::move(posse));
        }
        sk_X509_free(ca);
    }
    // O default da thread já foi restaurado aqui: a reimportação vai para o contexto global.
    return reimportar(brutas);
}

} // namespace

std::expected<PartesPfx, ErroCredencial> decifrarPfx(QByteArrayView pfx, const SenhaSegura& senha) {
    // NUL dentro da senha: a API do PKCS#12 lê uma C-string e truncaria, aceitando uma senha que o usuário não
    // digitou (A7 da revisão 2). Recusa antes do parse.
    if (senha.tamanho() > 0 && std::memchr(senha.comoC(), '\0', senha.tamanho()) != nullptr) {
        return std::unexpected(ErroCredencial::SenhaErrada);
    }
    auto primeira = tentar(pfx, senha, false);
    if (primeira) {
        return std::move(*primeira);
    }
    if (primeira.error() != Falha::Unsupported) {
        switch (primeira.error()) {
        case Falha::SenhaErrada: return std::unexpected(ErroCredencial::SenhaErrada);
        case Falha::Malformado: return std::unexpected(ErroCredencial::PfxMalformado);
        case Falha::LegadoIndisponivel: return std::unexpected(ErroCredencial::CifraLegadaIndisponivel);
        case Falha::Unsupported:
        case Falha::Interno: return std::unexpected(ErroCredencial::FalhaInterna);
        }
    }
    // Nova tentativa só porque o motivo foi "unsupported": cifra que o default não abre (RC2, 3DES).
    auto segunda = tentar(pfx, senha, true);
    if (segunda) {
        return std::move(*segunda);
    }
    switch (segunda.error()) {
    case Falha::SenhaErrada: return std::unexpected(ErroCredencial::SenhaErrada);
    case Falha::Malformado: return std::unexpected(ErroCredencial::PfxMalformado);
    case Falha::Unsupported:
    case Falha::LegadoIndisponivel: return std::unexpected(ErroCredencial::CifraLegadaIndisponivel);
    case Falha::Interno: return std::unexpected(ErroCredencial::FalhaInterna);
    }
    return std::unexpected(ErroCredencial::FalhaInterna);
}

} // namespace nl::foundation::pkcs12
