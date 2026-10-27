// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Validador XSD sem estado (NL-1.6a): confere um XML bem formado contra um Esquema verificado.
#ifndef NL_FOUNDATION_XML_VALIDADOR_XSD_H
#define NL_FOUNDATION_XML_VALIDADOR_XSD_H

#include <nl/foundation/xml/erros_xsd.h>
#include <nl/foundation/xml/esquema.h>

#include <QByteArrayView>

#include <expected>

namespace nl::foundation::xml {

/// Só funções estáticas: nenhum estado entre chamadas. Entidades externas e DTD não são carregados (sem XXE).
class ValidadorXsd
{
public:
    ValidadorXsd() = delete;

    static std::expected<void, ErrosXsd> validar(QByteArrayView xml, const Esquema& esquema);
};

} // namespace nl::foundation::xml

#endif // NL_FOUNDATION_XML_VALIDADOR_XSD_H
