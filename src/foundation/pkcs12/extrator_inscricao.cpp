// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Extração da inscrição (NL-2.2). OIDs do DOC-ICP-04 v8.0 (7.1.2.3): CNPJ no 2.16.76.1.3.3; CPF no 2.16.76.1.3.1,
// cujo conteúdo é data (8) seguida de CPF (11): o CPF ocupa as posições 9 a 19 (índices 8 a 18).
#include "extrator_inscricao.h"

#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/objects.h>
#include <openssl/x509v3.h>

#include <memory>
#include <string>

namespace nl::foundation::pkcs12 {

namespace {

constexpr const char* kOidCnpj = "2.16.76.1.3.3";
constexpr const char* kOidCpf = "2.16.76.1.3.1";
constexpr std::size_t kDeslocamentoCpf = 8;
constexpr std::size_t kTamanhoCpf = 11;

struct LiberadorNomes {
    void operator()(GENERAL_NAMES* nomes) const noexcept { GENERAL_NAMES_free(nomes); }
};
using NomesPtr = std::unique_ptr<GENERAL_NAMES, LiberadorNomes>;

struct LiberadorOid {
    void operator()(ASN1_OBJECT* oid) const noexcept { ASN1_OBJECT_free(oid); }
};
using OidPtr = std::unique_ptr<ASN1_OBJECT, LiberadorOid>;

/// Conteúdo OCTET STRING do otherName cujo tipo é o OID dado. nullopt se não houver.
std::optional<std::string> conteudoDoOtherName(GENERAL_NAMES* nomes, const char* oidTexto) {
    OidPtr alvo(OBJ_txt2obj(oidTexto, 1));
    if (!alvo) {
        ERR_clear_error();
        return std::nullopt;
    }
    const int quantidade = sk_GENERAL_NAME_num(nomes);
    for (int i = 0; i < quantidade; ++i) {
        GENERAL_NAME* nome = sk_GENERAL_NAME_value(nomes, i);
        if (nome->type != GEN_OTHERNAME || nome->d.otherName == nullptr) {
            continue;
        }
        const OTHERNAME* outro = nome->d.otherName;
        if (OBJ_cmp(outro->type_id, alvo.get()) != 0 || outro->value == nullptr) {
            continue;
        }
        if (ASN1_TYPE_get(outro->value) != V_ASN1_OCTET_STRING) {
            continue;
        }
        const ASN1_OCTET_STRING* octetos = outro->value->value.octet_string;
        const unsigned char* dados = ASN1_STRING_get0_data(octetos);
        const int tamanho = ASN1_STRING_length(octetos);
        if (dados == nullptr || tamanho < 0) {
            continue;
        }
        return std::string(reinterpret_cast<const char*>(dados), static_cast<std::size_t>(tamanho));
    }
    return std::nullopt;
}

} // namespace

std::optional<Inscricao> extrairInscricao(x509_st* certificado) {
    NomesPtr nomes(static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(certificado, NID_subject_alt_name, nullptr, nullptr)));
    ERR_clear_error();
    if (!nomes) {
        return std::nullopt;
    }
    if (auto cnpj = conteudoDoOtherName(nomes.get(), kOidCnpj)) {
        const QString texto = QString::fromLatin1(cnpj->data(), static_cast<qsizetype>(cnpj->size()));
        return Inscricao::cnpj(texto);
    }
    if (auto cpf = conteudoDoOtherName(nomes.get(), kOidCpf)) {
        if (cpf->size() < kDeslocamentoCpf + kTamanhoCpf) {
            return std::nullopt;
        }
        const QString texto = QString::fromLatin1(cpf->data() + kDeslocamentoCpf,
                                                  static_cast<qsizetype>(kTamanhoCpf));
        return Inscricao::cpf(texto);
    }
    return std::nullopt;
}

} // namespace nl::foundation::pkcs12
