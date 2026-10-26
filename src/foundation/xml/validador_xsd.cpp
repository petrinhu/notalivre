// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Validador XSD (NL-1.6a). Ver validador_xsd.h.
#include <nl/foundation/xml/validador_xsd.h>

#include "esquema_recurso.h"
#include "libxml_util.h"

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

    // Leitura sem NOENT e sem DTDLOAD: entidades externas e DTD não são carregados (sem XXE).
    // Sem NOBLANKS: o documento chega ao validador como o xmllint o lê, para o oráculo ser comparação justa.
    xmlInitParser();
    xmlResetLastError();
    std::unique_ptr<xmlDoc, detail::DocLiberador> documento{xmlReadMemory(
        xml.data(), static_cast<int>(xml.size()), "entrada.xml", nullptr, XML_PARSE_NONET)};
    if (!documento) {
        return std::unexpected(ErrosXsd{Motivo::xmlMalformado,
                                        {detail::erroDaUltimaLeitura("xml.malformado")}});
    }

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
