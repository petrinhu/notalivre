// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Definição privada de Esquema::Recurso (NL-1.6a): dono do xmlSchema da libxml2. Só os .cpp do motor incluem este arquivo.
#ifndef NL_FOUNDATION_XML_ESQUEMA_RECURSO_H
#define NL_FOUNDATION_XML_ESQUEMA_RECURSO_H

#include <nl/foundation/xml/esquema.h>

#include <libxml/xmlschemas.h>

namespace nl::foundation::xml {

class Esquema::Recurso
{
public:
    explicit Recurso(xmlSchema* esquema) noexcept : esquema_{esquema} {}
    ~Recurso() { xmlSchemaFree(esquema_); }

    Recurso(const Recurso&) = delete;
    Recurso& operator=(const Recurso&) = delete;

    xmlSchema* esquema() const noexcept { return esquema_; }

private:
    xmlSchema* esquema_;
};

} // namespace nl::foundation::xml

#endif // NL_FOUNDATION_XML_ESQUEMA_RECURSO_H
