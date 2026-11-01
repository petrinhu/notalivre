// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parse seguro do validador XSD (NL-2.21). Gêmeo do átomo do xmldsig (parse_xml_privado.cpp). Ver parse_seguro.h.
#include "parse_seguro.h"

#include <libxml/parserInternals.h>

#include <string>

namespace nl::foundation::xml::detail {
namespace {

// Estado de um parse, guardado na chave _private do contexto. Só o tratador do DOCTYPE escreve nele.
struct EstadoParseSeguro
{
    bool dtdVisto = false;
};

// Callback do DOCTYPE: marca que o DTD foi visto e para o parser antes de ler o subconjunto interno e o DTD externo.
void paradaNoDtd(void* ctx, const xmlChar* /*nome*/, const xmlChar* /*publicId*/, const xmlChar* /*systemId*/)
{
    auto* parser = static_cast<xmlParserCtxtPtr>(ctx);
    if (auto* estado = static_cast<EstadoParseSeguro*>(parser->_private); estado != nullptr) {
        estado->dtdVisto = true;
    }
    xmlStopParser(parser);
}

// Última falha da libxml2 desde o xmlResetLastError, sem o fim de linha. Usa xmlGetLastError (global), e não
// ctxt->lastError: esse membro é XML_DEPRECATED_MEMBER na 2.15 (Ubuntu 26.04), e com -Werror quebra o build (NL-2.21,
// log NL-2.21-verde-ubuntu26.04-e3c8275.log, linhas 497 a 536). xmlGetLastError é a API que o validador já usava.
std::string mensagemDaUltimaFalha()
{
    const xmlError* ultimo = xmlGetLastError();
    if (ultimo == nullptr || ultimo->message == nullptr) {
        return {};
    }
    std::string texto = ultimo->message;
    while (!texto.empty() && (texto.back() == '\n' || texto.back() == '\r' || texto.back() == ' ')) {
        texto.pop_back();
    }
    return "linha " + std::to_string(ultimo->line) + ": " + texto;
}

} // namespace

ContextoParseSeguro criarContextoParseSeguro(QByteArrayView xml)
{
    ContextoParseSeguro contexto(xmlCreateMemoryParserCtxt(xml.data(), static_cast<int>(xml.size())));
    if (!contexto) {
        return contexto;
    }
    contexto->sax->internalSubset = paradaNoDtd;
    // Camada 2: se a parada falhasse, NONET ainda bloqueia a rede. xmlCtxtSetOptions a partir da 2.13 (LIBXML_VERSION
    // 21300), xmlCtxtUseOptions antes; mesmo guard do átomo do xmldsig.
#if LIBXML_VERSION >= 21300
    xmlCtxtSetOptions(contexto.get(), XML_PARSE_NONET);
#else
    xmlCtxtUseOptions(contexto.get(), XML_PARSE_NONET);
#endif
    return contexto;
}

SaidaParseSeguro analisarParseSeguro(QByteArrayView xml)
{
    ContextoParseSeguro contexto = criarContextoParseSeguro(xml);
    if (!contexto) {
        return SaidaParseSeguro{nullptr, false, false, "a libxml2 não criou o contexto do parse"};
    }
    EstadoParseSeguro estado;
    contexto->_private = &estado;
    xmlResetLastError();
    xmlParseDocument(contexto.get());
    SaidaParseSeguro saida{std::unique_ptr<xmlDoc, DocLiberador>(contexto->myDoc),
                           contexto->wellFormed != 0,
                           estado.dtdVisto,
                           {}};
    contexto->myDoc = nullptr;  // o documento passa ao chamador; o contexto é liberado sem ele
    if (!saida.bemFormado && !saida.dtdVisto) {
        saida.mensagemDoErro = mensagemDaUltimaFalha();
    }
    return saida;
}

} // namespace nl::foundation::xml::detail
