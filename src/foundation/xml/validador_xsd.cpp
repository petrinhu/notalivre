// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Validador XSD (NL-1.6a). Ver validador_xsd.h.
#include <nl/foundation/xml/validador_xsd.h>

#include "esquema_recurso.h"
#include "libxml_util.h"
#include "parse_seguro.h"

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace nl::foundation::xml {
namespace {

using Motivo = ErrosXsd::Motivo;

ErrosXsd falhaInterna(std::string mensagem)
{
    return ErrosXsd{Motivo::falhaInterna, {nl::base::Erro{"xsd.interno", std::nullopt, std::move(mensagem)}}};
}

} // namespace

std::expected<void, ErrosXsd> ValidadorXsd::validar(QByteArrayView xml, const Esquema& esquema)
{
    if (xml.size() > std::numeric_limits<int>::max()) {
        return std::unexpected(ErrosXsd{Motivo::xmlMalformado,
                                        {nl::base::Erro{"xml.tamanho", std::nullopt,
                                                        "XML maior que o limite de 2 GiB da libxml2"}}});
    }

    // Parse seguro (NL-2.21, gêmeo do átomo do xmldsig): para no primeiro DOCTYPE, sem ler o DTD, e NONET como segunda
    // camada. Sem NOBLANKS: o documento chega ao validador como o xmllint o lê, para o oráculo ser comparação justa.
    xmlInitParser();
    detail::SaidaParseSeguro saida = detail::analisarParseSeguro(xml);
    if (saida.dtdVisto) {
        return std::unexpected(ErrosXsd{Motivo::xmlMalformado,
                                        {nl::base::Erro{"xml.com_dtd", std::nullopt,
                                                        "XML com DOCTYPE recusado: o validador não lê DTD"}}});
    }
    if (!saida.bemFormado || !saida.doc) {
        std::string mensagem = saida.mensagemDoErro.empty() ? std::string{"XML malformado (a libxml2 não deu detalhe)"}
                                                            : saida.mensagemDoErro;
        return std::unexpected(ErrosXsd{Motivo::xmlMalformado,
                                        {nl::base::Erro{"xml.malformado", std::nullopt, std::move(mensagem)}}});
    }
    std::unique_ptr<xmlDoc, detail::DocLiberador> documento = std::move(saida.doc);

    detail::ColetorErros coletor{"xsd.invalido"};
    std::unique_ptr<xmlSchemaValidCtxt, detail::ValidCtxtLiberador> contexto{
        xmlSchemaNewValidCtxt(esquema.recurso().esquema())};
    if (!contexto) {
        return std::unexpected(falhaInterna("libxml2 não criou o contexto de validação"));
    }
    xmlSchemaSetValidStructuredErrors(contexto.get(), detail::coletarErro, &coletor);

    const int resultado = xmlSchemaValidateDoc(contexto.get(), documento.get());
    if (resultado == 0) {
        return {};
    }
    if (resultado > 0) {
        return std::unexpected(ErrosXsd{Motivo::invalido, std::move(coletor.erros)});
    }
    return std::unexpected(falhaInterna("libxml2 falhou ao validar, retorno " + std::to_string(resultado)));
}

} // namespace nl::foundation::xml
