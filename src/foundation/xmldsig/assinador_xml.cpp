// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Assinador XMLDSIG enveloped (NL-2.3, DA-2.1, DA-2.2, DA-2.10, DA-2.36, DA-2.40). Fluxo da rodada 2: recusa DTD e XML já
// assinado; achado único do Id, comparado por inteiro, no infDPS filho da raiz; validade do certificado na hora de assinar;
// Signature inserida já na posição final, com digest e SignatureValue vazios; digest e SignedInfo canonicalizados com os
// xmlns herdados do documento (DA-2.40, itens 4 a 6). Nada aqui lê ou define a variável de SHA-1 (portão sha1_env).
#include <nl/foundation/xmldsig/assinador_xml.h>

#include <libxml/c14n.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>

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

// Elemento com o nome e o namespace dados. O namespace vazio significa elemento sem namespace.
bool ehElemento(xmlNodePtr no, std::string_view nome, std::string_view ns) {
    if (no == nullptr || no->type != XML_ELEMENT_NODE || std::string_view(reinterpret_cast<const char*>(no->name)) != nome) {
        return false;
    }
    if (no->ns == nullptr) {
        return ns.empty();
    }
    return std::string_view(reinterpret_cast<const char*>(no->ns->href)) == ns;
}

// Todos os elementos cujo atributo Id SEM namespace é exatamente o id (DA-2.40, item 2). Id com prefixo não conta
// (xmlGetNoNsProp, não xmlGetProp, que casaria só pelo nome local).
void coletarPorId(xmlNodePtr no, const std::string& id, std::vector<xmlNodePtr>& achados) {
    for (xmlNodePtr atual = no; atual != nullptr; atual = atual->next) {
        if (atual->type != XML_ELEMENT_NODE) {
            continue;
        }
        xmlChar* valor = xmlGetNoNsProp(atual, reinterpret_cast<const xmlChar*>("Id"));
        if (valor != nullptr) {
            if (id == reinterpret_cast<const char*>(valor)) {
                achados.push_back(atual);
            }
            xmlFree(valor);
        }
        coletarPorId(atual->children, id, achados);
    }
}

// Há algum ds:Signature no no ou abaixo dele? (DA-2.40, item 3.)
bool contemSignature(xmlNodePtr no) {
    for (xmlNodePtr atual = no; atual != nullptr; atual = atual->next) {
        if (atual->type != XML_ELEMENT_NODE) {
            continue;
        }
        if (ehElemento(atual, "Signature", kDsig) || contemSignature(atual->children)) {
            return true;
        }
    }
    return false;
}

// Conjunto de nós da C14N (DA-2.40 adendo, W3C 4.4.1): a subárvore do apice, sem as Signature quando o digest
// pede a transformada enveloped (W3C 6.6.4). Namespace e atributo são visíveis pelo elemento onde estão em escopo
// (o parent que a libxml2 passa), então o xmlns:xsi e o xml:lang da raiz entram quando o apice os herda.
struct ConjuntoDeNos {
    xmlNodePtr apice;
    bool excluirSignature;
};

bool dentroDeSignature(xmlNodePtr no) {
    for (xmlNodePtr atual = no; atual != nullptr; atual = atual->parent) {
        if (atual->type == XML_ELEMENT_NODE && ehElemento(atual, "Signature", kDsig)) {
            return true;
        }
    }
    return false;
}

bool dentroDoApice(xmlNodePtr no, xmlNodePtr apice) {
    for (xmlNodePtr atual = no; atual != nullptr; atual = atual->parent) {
        if (atual == apice) {
            return true;
        }
    }
    return false;
}

int noEhVisivel(void* dados, xmlNodePtr no, xmlNodePtr pai) {
    const auto* conjunto = static_cast<const ConjuntoDeNos*>(dados);
    const bool porPai = no->type == XML_NAMESPACE_DECL || no->type == XML_ATTRIBUTE_NODE;
    xmlNodePtr base = porPai ? pai : no;
    if (base == nullptr || !dentroDoApice(base, conjunto->apice)) {
        return 0;
    }
    return conjunto->excluirSignature && dentroDeSignature(base) ? 0 : 1;
}

// C14N 1.0 inclusiva pelo próprio documento do nó, com função de visibilidade (DA-2.40 adendo). Sem cópia do nó.
std::optional<std::string> canonicalizar(xmlNodePtr apice, bool excluirSignature) {
    ConjuntoDeNos conjunto{apice, excluirSignature};
    xmlOutputBufferPtr saida = xmlAllocOutputBuffer(nullptr);
    if (saida == nullptr) {
        return std::nullopt;
    }
    const int rc = xmlC14NExecute(apice->doc, noEhVisivel, &conjunto, XML_C14N_1_0, nullptr, 0, saida);
    std::optional<std::string> resultado;
    if (rc >= 0 && xmlOutputBufferGetContent(saida) != nullptr) {
        resultado = std::string(reinterpret_cast<const char*>(xmlOutputBufferGetContent(saida)),
                                static_cast<std::size_t>(xmlOutputBufferGetSize(saida)));
    }
    xmlOutputBufferClose(saida);
    return resultado;
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

// Validade na hora de assinar (DA-2.40, item 8): notBefore já passou e notAfter ainda não.
bool validadeDoMaterial(const x509_st* certificado) {
    return X509_cmp_current_time(X509_get0_notBefore(certificado)) <= 0 &&
           X509_cmp_current_time(X509_get0_notAfter(certificado)) > 0;
}

xmlNodePtr filhoPorNome(xmlNodePtr pai, std::string_view nome) {
    for (xmlNodePtr atual = pai->children; atual != nullptr; atual = atual->next) {
        if (ehElemento(atual, nome, kDsig)) {
            return atual;
        }
    }
    return nullptr;
}

// Modelo da Signature na posição final (DA-2.40, item 4): DigestValue e SignatureValue vazios, KeyInfo já preenchido.
std::string modeloSignature(const ParametrosAssinatura& parametros, const std::string& id,
                            const std::string& certificadoB64) {
    const std::string dsig(kDsig);
    const std::string c14n(kC14n);
    const std::string env(kEnveloped);
    return "<Signature xmlns=\"" + dsig + "\"><SignedInfo><CanonicalizationMethod Algorithm=\"" + c14n +
           "\"/><SignatureMethod Algorithm=\"" + std::string(parametros.uriAssinatura) + "\"/><Reference URI=\"#" + id +
           "\"><Transforms><Transform Algorithm=\"" + env + "\"/><Transform Algorithm=\"" + c14n +
           "\"/></Transforms><DigestMethod Algorithm=\"" + std::string(parametros.uriDigest) +
           "\"/><DigestValue></DigestValue></Reference></SignedInfo><SignatureValue></SignatureValue><KeyInfo>"
           "<X509Data><X509Certificate>" + certificadoB64 + "</X509Certificate></X509Data></KeyInfo></Signature>";
}

} // namespace

std::string_view codigo(ErroAssinatura erro) noexcept {
    switch (erro) {
    case ErroAssinatura::XmlMalformado:
        return "assinatura.xml_malformado";
    case ErroAssinatura::XmlComDtd:
        return "assinatura.xml_com_dtd";
    case ErroAssinatura::ReferenciaNaoEncontrada:
        return "assinatura.referencia_nao_encontrada";
    case ErroAssinatura::ReferenciaAmbigua:
        return "assinatura.referencia_ambigua";
    case ErroAssinatura::JaAssinado:
        return "assinatura.ja_assinado";
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
    if (!validadeDoMaterial(material.certificado)) {
        return std::unexpected(ErroAssinatura::MaterialInvalido);
    }

    const std::string id = idReferencia.toUtf8().toStdString();
    Doc doc(xmlReadMemory(xml.data(), static_cast<int>(xml.size()), "dps.xml", nullptr, XML_PARSE_NONET));
    if (!doc) {
        return std::unexpected(ErroAssinatura::XmlMalformado);
    }
    if (doc->intSubset != nullptr || doc->extSubset != nullptr) {
        return std::unexpected(ErroAssinatura::XmlComDtd);  // DTD e entidade recusados antes de qualquer uso (item 1)
    }
    xmlNodePtr raiz = xmlDocGetRootElement(doc.get());
    if (raiz == nullptr) {
        return std::unexpected(ErroAssinatura::XmlMalformado);
    }
    if (contemSignature(raiz)) {
        return std::unexpected(ErroAssinatura::JaAssinado);  // reassinar exige o XML sem assinatura (item 3)
    }

    std::vector<xmlNodePtr> achados;
    coletarPorId(raiz, id, achados);
    if (achados.size() > 1) {
        return std::unexpected(ErroAssinatura::ReferenciaAmbigua);
    }
    if (achados.empty()) {
        return std::unexpected(ErroAssinatura::ReferenciaNaoEncontrada);
    }
    xmlNodePtr referencia = achados.front();
    if (!ehElemento(referencia, parametros.nomeReferencia, parametros.namespaceLeiaute) || referencia->parent != raiz ||
        !ehElemento(raiz, parametros.nomePaiReferencia, parametros.namespaceLeiaute)) {
        return std::unexpected(ErroAssinatura::ReferenciaNaoEncontrada);  // Id fora do infDPS filho da raiz DPS (item 2)
    }

    LibCtx ctx(OSSL_LIB_CTX_new());
    if (!ctx) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    Provedor provedor(OSSL_PROVIDER_load(ctx.get(), "default"));  // provider default explícito (DA-2.10)
    if (!provedor) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }

    const auto certificado = certificadoEmDer(material.certificado);
    if (!certificado) {
        return std::unexpected(certificado.error());
    }
    // Modelo da Signature já na posição final, irmã do infDPS (item 4). Digest e SignatureValue são preenchidos depois.
    Doc modelo(xmlReadMemory(modeloSignature(parametros, id, *certificado).c_str(),
                             static_cast<int>(modeloSignature(parametros, id, *certificado).size()), "signature.xml",
                             nullptr, XML_PARSE_NONET));
    if (!modelo) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    xmlNodePtr assinatura = xmlDocCopyNode(xmlDocGetRootElement(modelo.get()), doc.get(), 1);
    if (assinatura == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    xmlAddNextSibling(referencia, assinatura);

    // Digest da referência: subárvore do infDPS, sem a Signature (enveloped), no contexto do documento (item 5).
    const auto canonicoReferencia = canonicalizar(referencia, /*excluirSignature=*/true);
    if (!canonicoReferencia) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const auto digestReferencia = digest(ctx.get(), *nomeDigest, *canonicoReferencia);
    if (!digestReferencia) {
        return std::unexpected(digestReferencia.error());
    }
    const std::string digestB64 = base64(digestReferencia->data(), digestReferencia->size());
    xmlNodePtr signedInfo = filhoPorNome(assinatura, "SignedInfo");
    xmlNodePtr reference = signedInfo != nullptr ? filhoPorNome(signedInfo, "Reference") : nullptr;
    xmlNodePtr digestNo = reference != nullptr ? filhoPorNome(reference, "DigestValue") : nullptr;
    if (digestNo == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    xmlNodeSetContent(digestNo, reinterpret_cast<const xmlChar*>(digestB64.c_str()));

    // SignedInfo canonicalizado dentro do documento, com o xmlns:xsi da raiz herdado (item 6, achado mH).
    const auto canonicoSignedInfo = canonicalizar(signedInfo, /*excluirSignature=*/false);
    if (!canonicoSignedInfo) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const auto assinado = assinarRsa(ctx.get(), *nomeSig, const_cast<evp_pkey_st*>(material.chave), *canonicoSignedInfo);
    if (!assinado) {
        return std::unexpected(assinado.error());
    }
    xmlNodePtr valor = filhoPorNome(assinatura, "SignatureValue");
    if (valor == nullptr) {
        return std::unexpected(ErroAssinatura::FalhaInterna);
    }
    const std::string assinaturaB64 = base64(assinado->data(), assinado->size());
    xmlNodeSetContent(valor, reinterpret_cast<const xmlChar*>(assinaturaB64.c_str()));

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
