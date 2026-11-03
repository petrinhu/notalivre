// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parse do XML do assinador (NL-2.9, decisão do CISO de 05:34): dois átomos privados ao motor xmldsig. Não é instalado
// (como o decifrar_pfx.h do pkcs12); só o assinador e os testes do próprio motor o incluem.
#ifndef NL_FOUNDATION_XMLDSIG_PARSE_XML_PRIVADO_H
#define NL_FOUNDATION_XMLDSIG_PARSE_XML_PRIVADO_H

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <QByteArrayView>

#include <memory>
#include <optional>

namespace nl::foundation::xmldsig::privado {

struct LiberaContexto {
    void operator()(xmlParserCtxtPtr ctx) const noexcept { xmlFreeParserCtxt(ctx); }
};
using ContextoParser = std::unique_ptr<xmlParserCtxt, LiberaContexto>;

struct LiberaDocumento {
    void operator()(xmlDocPtr doc) const noexcept { xmlFreeDoc(doc); }
};
using DocumentoXml = std::unique_ptr<xmlDoc, LiberaDocumento>;

/// Resultado do parse sem DTD. posicaoDoCorpo fica vazia quando o parser nunca chegou ao primeiro elemento do corpo: a
/// parada no DOCTYPE é observável por essa ausência, porque o libxml2 esvazia a entrada ao parar (a posição em bytes
/// não é lida depois da parada).
struct SaidaDoParse {
    DocumentoXml doc;
    bool bemFormado;
    std::optional<long> posicaoDoCorpo;
};

/// Contexto do parser com a camada 2 (XML_PARSE_NONET) e o tratador SAX da parada no DOCTYPE. nullptr se a libxml2 não
/// alocar o contexto.
[[nodiscard]] ContextoParser criarContextoDoParser(QByteArrayView xml);

/// Analisa o XML parando no primeiro DOCTYPE. Sem RECOVER: documento mal formado devolve bemFormado = false.
[[nodiscard]] SaidaDoParse analisarSemDtd(QByteArrayView xml);

} // namespace nl::foundation::xmldsig::privado

#endif // NL_FOUNDATION_XMLDSIG_PARSE_XML_PRIVADO_H
