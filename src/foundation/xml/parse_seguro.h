// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parse seguro do validador XSD (NL-2.21, DA-2.67): gêmeo do átomo do xmldsig (src/foundation/xmldsig/parse_xml_privado.h).
// Privado ao motor xml: só os .cpp de src/foundation/xml e os testes do motor o incluem. Os motores não se incluem entre si
// (DA-2.30), por isso a camada se repete aqui, em duas ocorrências, abaixo da regra de 3 da L-33.
// Duas camadas: a parada no primeiro DOCTYPE (o DTD nem é lido) e XML_PARSE_NONET (sem rede, se a parada falhasse).
#ifndef NL_FOUNDATION_XML_PARSE_SEGURO_H
#define NL_FOUNDATION_XML_PARSE_SEGURO_H

#include "libxml_util.h"

#include <libxml/parser.h>

#include <QByteArrayView>

#include <memory>
#include <string>

namespace nl::foundation::xml::detail {

struct LiberaContextoSeguro
{
    void operator()(xmlParserCtxtPtr contexto) const noexcept { xmlFreeParserCtxt(contexto); }
};
using ContextoParseSeguro = std::unique_ptr<xmlParserCtxt, LiberaContextoSeguro>;

/// Resultado do parse seguro. dtdVisto = o parser parou no DOCTYPE e o documento não foi lido. bemFormado = false quando
/// a sintaxe falhou; mensagemDoErro traz a última falha do próprio contexto ("linha N: ..."), vazia se a libxml2 não deu.
struct SaidaParseSeguro
{
    std::unique_ptr<xmlDoc, DocLiberador> doc;
    bool bemFormado;
    bool dtdVisto;
    std::string mensagemDoErro;
};

/// Contexto com NONET e a parada no DOCTYPE. nullptr se a libxml2 não alocar o contexto.
[[nodiscard]] ContextoParseSeguro criarContextoParseSeguro(QByteArrayView xml);

/// Lê o XML parando no primeiro DOCTYPE. Sem RECOVER: documento mal formado devolve bemFormado = false.
[[nodiscard]] SaidaParseSeguro analisarParseSeguro(QByteArrayView xml);

} // namespace nl::foundation::xml::detail

#endif // NL_FOUNDATION_XML_PARSE_SEGURO_H
