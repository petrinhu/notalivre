// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Assinador XMLDSIG enveloped (NL-2.3, DA-2.1, DA-2.2, DA-2.10, DA-2.36). Fluxo: parse, localizar o Id, C14N 1.0 da
// subárvore (com os xmlns herdados declarados numa cópia), digest, SignedInfo canonicalizado, RSA com OSSL_LIB_CTX próprio
// (provider default carregado explicitamente), Signature irmã do elemento, KeyInfo com o X509Certificate em DER.
// Nada aqui lê ou define a variável de SHA-1 (portão sha1_env).
#include <nl/foundation/xmldsig/assinador_xml.h>

#include <libxml/c14n.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/x509.h>

#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nl::foundation::xmldsig {
namespace {

constexpr std::string_view kDsig = "http://www.w3.org/2000/09/xmldsig#";
constexpr std::string_view kC14n = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
constexpr std::string_view kEnveloped = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";

struct LibCtxLiberar {
    void operator()(OSSL_LIB_CTX* ctx) const noexcept { OSSL_LIB_CTX_free(ctx); }
};
using LibCtx = std::unique_ptr<OSSL_LIB_CTX, LibCtxLiberar>;

struct DocLiberar {
    void operator()(xmlDocPtr doc) const noexcept { xmlFreeDoc(doc); }
};
using Doc = std::unique_ptr<xmlDoc, DocLiberar>;

struct ProvedorLiberar {
    void operator()(OSSL_PROVIDER* prov) const noexcept { OSSL_PROVIDER_unload(prov); }
};
using Provedor = std::unique_ptr<OSSL_PROVIDER, ProvedorLiberar>;

// URI da tabela do motor -> nome do OpenSSL. Fora da tabela, ou recusado pelo provider: algoritmo bloqueado (DA-2.10).
std::optional<std::string> nomeDoDigest(std::string_view uri) {
    if (uri == "http://www.w3.org/2000/09/xmldsig#sha1") {
        return "SHA1";
    }
    if (uri == "http://www.w3.org/2001/04/xmlenc#sha256") {
        return "SHA256";
    }
    return std::nullopt;
}

std::optional<std::string> nomeDaAssinatura(std::string_view uri) {
    if (uri == "http://www.w3.org/2000/09/xmldsig#rsa-sha1") {
        return "SHA1";
    }
    if (uri == "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256") {
        return "SHA256";
    }
    return std::nullopt;
}

xmlNodePtr encontrarPorId(xmlNodePtr no, const std::string& id) {
    for (xmlNodePtr atual = no; atual != nullptr; atual = atual->next) {
        if (atual->type != XML_ELEMENT_NODE) {
            continue;
        }
        xmlChar* valor = xmlGetProp(atual, reinterpret_cast<const xmlChar*>("Id"));
        const bool achou = valor != nullptr && id == reinterpret_cast<const char*>(valor);
        if (valor != nullptr) {
            xmlFree(valor);
        }
        if (achou) {
            return atual;
        }
        if (xmlNodePtr filho = encontrarPorId(atual->children, id)) {
            return filho;
        }
    }
    return nullptr;
}

// C14N 1.0 inclusiva da subárvore. A cópia leva os xmlns herdados dos ancestrais, como exige o subconjunto (DA-2.2).
std::optional<std::string> canonicalizar(xmlNodePtr no) {
    Doc copia(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")));
    if (!copia) {
        return std::nullopt;
    }
    xmlNodePtr raiz = xmlDocCopyNode(no, copia.get(), 1);
    if (raiz == nullptr) {
        return std::nullopt;
    }
    xmlDocSetRootElement(copia.get(), raiz);
    for (xmlNodePtr ancestral = no->parent; ancestral != nullptr && ancestral->type == XML_ELEMENT_NODE;
         ancestral = ancestral->parent) {
        for (xmlNsPtr ns = ancestral->nsDef; ns != nullptr; ns = ns->next) {
            if (xmlSearchNs(copia.get(), raiz, ns->prefix) == nullptr) {
                xmlNewNs(raiz, ns->href, ns->prefix);
            }
        }
    }
    xmlChar* bytes = nullptr;
    const int tamanho = xmlC14NDocDumpMemory(copia.get(), nullptr, XML_C14N_1_0, nullptr, 0, &bytes);
    if (tamanho < 0 || bytes == nullptr) {
        return std::nullopt;
    }
    std::string saida(reinterpret_cast<const char*>(bytes), static_cast<std::size_t>(tamanho));
    xmlFree(bytes);
    return saida;
}

std::string base64(const unsigned char* dados, std::size_t tamanho) {
    std::string saida(4 * ((tamanho + 2) / 3) + 1, '\0');
    const int n = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(saida.data()), dados, static_cast<int>(tamanho));
    saida.resize(static_cast<std::size_t>(n));
    return saida;
}

std::expected<std::vector<unsigned char>, ErroAssinatura> digest(OSSL_LIB_CTX* ctx, const std::string& nome,
                                                                 const std::string& dados) {
    EVP_MD* md = EVP_MD_fetch(ctx, nome.c_str(), nullptr);
    if (md == nullptr) {
        return std::unexpected(ErroAssinatura::AlgoritmoBloqueado);
    }
    std::vector<unsigned char> saida(EVP_MAX_MD_SIZE);
    unsigned int tamanho = 0;
    const int ok = EVP_Digest(dados.data(), dados.size(), saida.data(), &tamanho, md, nullptr);
    EVP_MD_free(md);
    if (ok != 1) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    saida.resize(tamanho);
    return saida;
}

std::expected<std::vector<unsigned char>, ErroAssinatura> assinarRsa(OSSL_LIB_CTX* ctx, const std::string& nome,
                                                                     evp_pkey_st* chave, const std::string& dados) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    std::expected<std::vector<unsigned char>, ErroAssinatura> resultado = std::unexpected(ErroAssinatura::FalhaInterna);
    if (EVP_DigestSignInit_ex(mdctx, nullptr, nome.c_str(), ctx, nullptr, chave, nullptr) != 1) {
        resultado = std::unexpected(ErroAssinatura::AlgoritmoBloqueado);
    } else if (EVP_DigestSignUpdate(mdctx, dados.data(), dados.size()) == 1) {
        std::size_t tamanho = 0;
        if (EVP_DigestSignFinal(mdctx, nullptr, &tamanho) == 1) {
            std::vector<unsigned char> sig(tamanho);
            if (EVP_DigestSignFinal(mdctx, sig.data(), &tamanho) == 1) {
                sig.resize(tamanho);
                resultado = std::move(sig);
            }
        }
    }
    EVP_MD_CTX_free(mdctx);
    return resultado;
}

std::expected<std::string, ErroAssinatura> certificadoEmDer(const x509_st* certificado) {
    unsigned char* der = nullptr;
    const int tamanho = i2d_X509(const_cast<x509_st*>(certificado), &der);
    if (tamanho <= 0 || der == nullptr) {
        return std::unexpected(ErroAssinatura::MaterialInvalido);
    }
    std::string saida = base64(der, static_cast<std::size_t>(tamanho));
    OPENSSL_free(der);
    return saida;
}

} // namespace

std::string_view codigo(ErroAssinatura erro) noexcept {
    switch (erro) {
    case ErroAssinatura::XmlMalformado:
        return "assinatura.xml_malformado";
    case ErroAssinatura::ReferenciaNaoEncontrada:
        return "assinatura.referencia_nao_encontrada";
    case ErroAssinatura::AlgoritmoBloqueado:
        return "assinatura.algoritmo_bloqueado";
    case ErroAssinatura::MaterialInvalido:
        return "assinatura.material_invalido";
    case ErroAssinatura::FalhaInterna:
        return "assinatura.falha_interna";
    }
    return "assinatura.falha_interna";
}

std::expected<QByteArray, ErroAssinatura> AssinadorXml::assinarEnveloped(QByteArrayView xml, QStringView idReferencia,
                                                                         const ParametrosAssinatura& parametros,
                                                                         const MaterialAssinatura& material) {
    if (material.certificado == nullptr || material.chave == nullptr) {
        return std::unexpected(ErroAssinatura::MaterialInvalido);
    }
    if (parametros.uriC14n != kC14n || parametros.uriEnveloped != kEnveloped) {
        return std::unexpected(ErroAssinatura::FalhaInterna);  // só C14N 1.0 e enveloped: o que o leiaute usa (DA-2.2)
    }
    const auto nomeDigest = nomeDoDigest(parametros.uriDigest);
    const auto nomeSig = nomeDaAssinatura(parametros.uriAssinatura);
    if (!nomeDigest || !nomeSig) {
        return std::unexpected(ErroAssinatura::AlgoritmoBloqueado);
    }

    const std::string id = idReferencia.toUtf8().toStdString();
    Doc doc(xmlReadMemory(xml.data(), static_cast<int>(xml.size()), "dps.xml", nullptr,
                          XML_PARSE_NONET));
    if (!doc) {
        return std::unexpected(ErroAssinatura::XmlMalformado);
    }
    xmlNodePtr referencia = encontrarPorId(xmlDocGetRootElement(doc.get()), id);
    if (referencia == nullptr) {
        return std::unexpected(ErroAssinatura::ReferenciaNaoEncontrada);
    }

    LibCtx ctx(OSSL_LIB_CTX_new());
    if (!ctx) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    Provedor provedor(OSSL_PROVIDER_load(ctx.get(), "default"));  // provider default explícito (DA-2.10)
    if (!provedor) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }

    const auto canonicoReferencia = canonicalizar(referencia);
    if (!canonicoReferencia) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const auto digestReferencia = digest(ctx.get(), *nomeDigest, *canonicoReferencia);
    if (!digestReferencia) {
        return std::unexpected(digestReferencia.error());
    }
    const std::string digestB64 = base64(digestReferencia->data(), digestReferencia->size());

    const std::string signedInfo =
        "<SignedInfo xmlns=\"" + std::string(kDsig) + "\"><CanonicalizationMethod Algorithm=\"" + std::string(kC14n) +
        "\"/><SignatureMethod Algorithm=\"" + std::string(parametros.uriAssinatura) +
        "\"/><Reference URI=\"#" + id + "\"><Transforms><Transform Algorithm=\"" + std::string(kEnveloped) +
        "\"/><Transform Algorithm=\"" + std::string(kC14n) + "\"/></Transforms><DigestMethod Algorithm=\"" +
        std::string(parametros.uriDigest) + "\"/><DigestValue>" + digestB64 +
        "</DigestValue></Reference></SignedInfo>";
    Doc si(xmlReadMemory(signedInfo.data(), static_cast<int>(signedInfo.size()), "signedinfo.xml", nullptr,
                         XML_PARSE_NONET));
    if (!si) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const auto canonicoSignedInfo = canonicalizar(xmlDocGetRootElement(si.get()));
    if (!canonicoSignedInfo) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const auto assinatura = assinarRsa(ctx.get(), *nomeSig, const_cast<evp_pkey_st*>(material.chave), *canonicoSignedInfo);
    if (!assinatura) {
        return std::unexpected(assinatura.error());
    }
    const auto certificado = certificadoEmDer(material.certificado);
    if (!certificado) {
        return std::unexpected(certificado.error());
    }

    const std::string fragmento =
        "<Signature xmlns=\"" + std::string(kDsig) + "\">" + signedInfo + "<SignatureValue>" +
        base64(assinatura->data(), assinatura->size()) + "</SignatureValue><KeyInfo><X509Data><X509Certificate>" +
        *certificado + "</X509Certificate></X509Data></KeyInfo></Signature>";
    Doc frag(xmlReadMemory(fragmento.data(), static_cast<int>(fragmento.size()), "signature.xml", nullptr,
                           XML_PARSE_NONET));
    if (!frag) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    xmlNodePtr copia = xmlDocCopyNode(xmlDocGetRootElement(frag.get()), doc.get(), 1);
    if (copia == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    xmlAddNextSibling(referencia, copia);  // Signature irmã do elemento assinado, dentro do elemento raiz (DA-2.3)

    xmlChar* saida = nullptr;
    int tamanho = 0;
    xmlDocDumpMemory(doc.get(), &saida, &tamanho);
    if (saida == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    QByteArray resultado(reinterpret_cast<const char*>(saida), tamanho);
    xmlFree(saida);
    return resultado;
}

} // namespace nl::foundation::xmldsig
