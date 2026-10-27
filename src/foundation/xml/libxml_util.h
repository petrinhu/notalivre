// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Auxiliares privados da libxml2 (NL-1.6a): coletor de erros por userData, desalocadores e campo citado na mensagem.
// Único estado global tocado: xmlGetLastError (por thread), só na leitura malformada. Pendência NL-1.6b: parser ctxt com erro próprio.
// Só pode ser incluído pelos .cpp de src/foundation/xml; nunca pelo cabeçalho público.
#ifndef NL_FOUNDATION_XML_LIBXML_UTIL_H
#define NL_FOUNDATION_XML_LIBXML_UTIL_H

#include <nl/base/erro.h>

#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlschemas.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nl::foundation::xml::detail {

// A assinatura do callback de erro mudou na libxml2 2.12 (passou a receber const xmlError*).
// O mint22.3 traz a 2.9.14; as outras imagens, 2.12 a 2.15.
#if LIBXML_VERSION >= 21200
using ErroLibxml = const xmlError*;
#else
using ErroLibxml = xmlErrorPtr;
#endif

/// Erros de uma chamada à libxml2. Vai no userData do callback: o coletor não é global.
struct ColetorErros
{
    explicit ColetorErros(std::string codigoPadrao) : codigo{std::move(codigoPadrao)} {}

    std::string codigo;
    std::vector<nl::base::Erro> erros;
};

/// Callback único de parser e de validador: empilha o erro no ColetorErros recebido em userData.
void coletarErro(void* userData, ErroLibxml erro);

/// Erro da última leitura da libxml2 (xmlGetLastError), com o código dado.
nl::base::Erro erroDaUltimaLeitura(std::string codigo);

/// Campo do erro, decidido por formato da mensagem da libxml2 (NL-1.6a):
///  - "Expected is ( {ns}dhEmi )": devolve o ESPERADO (dhEmi), que é o campo do leiaute a preencher. O elemento que
///    apareceu no lugar (verAplic) continua na mensagem. Vale para elemento obrigatório ausente.
///  - sem "Expected is", com "Element '{ns}X'": devolve X, o elemento onde a regra falhou (ex.: pattern de Id).
///  - nenhum dos dois: nullopt.
/// Testes: tst_validador_xsd (campoObrigatorioAusenteReprova e idForaDoPatternReprova) cobrem os dois formatos.
std::optional<std::string> campoDaMensagem(std::string_view mensagem);

struct DocLiberador
{
    void operator()(xmlDoc* documento) const noexcept { xmlFreeDoc(documento); }
};

struct ValidCtxtLiberador
{
    void operator()(xmlSchemaValidCtxt* contexto) const noexcept { xmlSchemaFreeValidCtxt(contexto); }
};

struct ParserCtxtLiberador
{
    void operator()(xmlSchemaParserCtxt* contexto) const noexcept { xmlSchemaFreeParserCtxt(contexto); }
};

struct SchemaLiberador
{
    void operator()(xmlSchema* esquema) const noexcept { xmlSchemaFree(esquema); }
};

} // namespace nl::foundation::xml::detail

#endif // NL_FOUNDATION_XML_LIBXML_UTIL_H
