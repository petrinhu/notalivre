// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Orquestração do carregamento de A1 (NL-2.2). Ordem fixa: decifrar, vencimento da folha, início da validade da
// folha, cadeia até as âncoras de fora (com validade das intermediárias), EKU, inscrição.
// Cada critério devolve o próprio ErroCredencial; nenhum depende da mensagem de erro do OpenSSL.
#include <nl/foundation/pkcs12/carregador_pkcs12.h>

#include "decifrar_pfx.h"
#include <nl/foundation/pkcs12/conferencia_chave.h>
#include "extrator_inscricao.h"
#include "verificador_cadeia.h"

#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <utility>

namespace nl::foundation::pkcs12 {

namespace {


// notAfter anterior ao instante atual.
bool vencido(x509_st* certificado) {
    return X509_cmp_current_time(X509_get0_notAfter(certificado)) < 0;
}

// notBefore posterior ao instante atual.
bool aindaNaoValido(x509_st* certificado) {
    return X509_cmp_current_time(X509_get0_notBefore(certificado)) > 0;
}

// Devolve true se a folha não tem extensão EKU. Presente e sem clientAuth, ou presente e malformada: erro.
// A extensão que existe e não decodifica nunca conta como ausente (revisão 1, A6).
std::expected<bool, ErroCredencial> verificarEku(x509_st* folha) {
    if (X509_get_ext_by_NID(folha, NID_ext_key_usage, -1) < 0) {
        return true;
    }
    auto* usos = static_cast<EXTENDED_KEY_USAGE*>(X509_get_ext_d2i(folha, NID_ext_key_usage, nullptr, nullptr));
    if (usos == nullptr) {
        ERR_clear_error();
        return std::unexpected(ErroCredencial::SemAutenticacaoCliente);
    }
    bool clientAuth = false;
    const int quantidade = sk_ASN1_OBJECT_num(usos);
    for (int i = 0; i < quantidade; ++i) {
        if (OBJ_obj2nid(sk_ASN1_OBJECT_value(usos, i)) == NID_client_auth) {
            clientAuth = true;
        }
    }
    EXTENDED_KEY_USAGE_free(usos);
    if (!clientAuth) {
        return std::unexpected(ErroCredencial::SemAutenticacaoCliente);
    }
    return false;
}

} // namespace

std::expected<CredencialX509, ErroCredencial> CarregadorPkcs12::carregar(QByteArrayView pfx, SenhaSegura& senha,
                                                                         const Ancoras& ancoras) {
    auto partes = decifrarPfx(pfx, senha);
    // A senha sai de uso logo após o parse, com sucesso ou falha. O chamador vê o próprio objeto zerado.
    senha.zerar();
    if (!partes) {
        return std::unexpected(partes.error());
    }

    x509_st* folha = partes->certificado.get();
    // A chave tem de pertencer à folha: PKCS#12 íntegro com a chave de outro certificado não é credencial (rodada 3).
    if (!chaveConfereComCertificado(folha, partes->chave.get())) {
        return std::unexpected(ErroCredencial::PfxMalformado);
    }
    if (vencido(folha)) {
        return std::unexpected(ErroCredencial::Vencido);
    }
    if (aindaNaoValido(folha)) {
        return std::unexpected(ErroCredencial::AindaNaoValido);
    }

    // EKU da folha antes da cadeia: a EKU malformada é recusada como SemAutenticacaoCliente, e não como
    // CadeiaInvalida pela verificação do OpenSSL, que também olha extensões (achado do vermelho/verde da rodada 2).
    auto ekuAusente = verificarEku(folha);
    if (!ekuAusente) {
        return std::unexpected(ekuAusente.error());
    }

    if (auto cadeia = verificarCadeia(folha, partes->intermediarios, ancoras); !cadeia) {
        return std::unexpected(cadeia.error());
    }

    auto inscricao = extrairInscricao(folha);
    if (!inscricao) {
        return std::unexpected(ErroCredencial::SemInscricao);
    }

    // Sem evento aqui: o adaptador do Mid registra credencial.eku_ausente pelo Log (DA-2.30).
    return CredencialX509(std::move(partes->certificado), std::move(partes->chave), std::move(*inscricao),
                          *ekuAusente);
}

std::expected<CredencialX509, ErroCredencial> CarregadorPkcs12::carregarConsumindo(QByteArrayView pfx, SenhaSegura&& senha,
                                                                                  const Ancoras& ancoras)
{
    // A senha é movida para um local: quem passou não tem mais o objeto, e o local é zerado ao sair da função.
    SenhaSegura consumida = std::move(senha);
    return carregar(pfx, consumida, ancoras);
}

} // namespace nl::foundation::pkcs12
