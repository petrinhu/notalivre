// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parse do XML do assinador (NL-2.9, decisão do CISO de 05:34). Ver parse_xml_privado.h.
#include "parse_xml_privado.h"

#include <libxml/SAX2.h>
#include <libxml/parserInternals.h>

namespace nl::foundation::xmldsig::privado {
namespace {

// Estado de um parse, guardado na chave _private do contexto. Só o tratador de startElement escreve nele.
struct EstadoDoParse {
    std::optional<long> posicaoDoCorpo;
};

// Callback do DOCTYPE (NL-2.9, condição (d)): cria o nó DTD como o padrão e para o parser antes de ler o subconjunto
// interno. Declarações (entidades em cascata, sintaxe ruim) nunca são processadas.
void paradaNoDtd(void* ctx, const xmlChar* nome, const xmlChar* publicId, const xmlChar* systemId) {
    xmlSAX2InternalSubset(ctx, nome, publicId, systemId);
    xmlStopParser(static_cast<xmlParserCtxtPtr>(ctx));
}

// Primeiro elemento do corpo: marca que o parser chegou lá, com a posição em bytes, e segue o parse normal.
void marcaChegadaAoCorpo(void* ctx, const xmlChar* local, const xmlChar* prefixo, const xmlChar* uri, int nbNamespaces,
                         const xmlChar** namespaces, int nbAtributos, int nbPadroes, const xmlChar** atributos) {
    auto* parser = static_cast<xmlParserCtxtPtr>(ctx);
    if (auto* estado = static_cast<EstadoDoParse*>(parser->_private); estado != nullptr && !estado->posicaoDoCorpo) {
        const long consumido = static_cast<long>(parser->input->consumed);
        estado->posicaoDoCorpo = consumido + static_cast<long>(parser->input->cur - parser->input->base);
    }
    xmlSAX2StartElementNs(ctx, local, prefixo, uri, nbNamespaces, namespaces, nbAtributos, nbPadroes, atributos);
}

} // namespace

ContextoParser criarContextoDoParser(QByteArrayView xml) {
    ContextoParser ctx(xmlCreateMemoryParserCtxt(xml.data(), static_cast<int>(xml.size())));
    if (!ctx) {
        return ctx;
    }
    ctx->sax->internalSubset = paradaNoDtd;
    ctx->sax->startElementNs = marcaChegadaAoCorpo;
    // Camada 2: se a parada falhasse, NONET ainda bloqueia o carregamento de rede. A API de opções muda por versão:
    // xmlCtxtSetOptions a partir da 2.13 (LIBXML_VERSION 21300), xmlCtxtUseOptions antes (ex.: Mint 22.3, a confirmar).
#if LIBXML_VERSION >= 21300
    xmlCtxtSetOptions(ctx.get(), XML_PARSE_NONET);
#else
    xmlCtxtUseOptions(ctx.get(), XML_PARSE_NONET);
#endif
    return ctx;
}

SaidaDoParse analisarSemDtd(QByteArrayView xml) {
    ContextoParser ctx = criarContextoDoParser(xml);
    if (!ctx) {
        return SaidaDoParse{DocumentoXml(), false, std::nullopt};
    }
    EstadoDoParse estado;
    ctx->_private = &estado;
    xmlParseDocument(ctx.get());
    DocumentoXml doc(ctx->myDoc);
    ctx->myDoc = nullptr;  // o documento passa ao chamador; o contexto é liberado sem ele
    return SaidaDoParse{std::move(doc), ctx->wellFormed != 0, estado.posicaoDoCorpo};
}

} // namespace nl::foundation::xmldsig::privado
