// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Esquema XSD verificado (NL-1.6a). Só nasce depois de conferir o SHA-256 de cada arquivo do manifesto.
#ifndef NL_FOUNDATION_XML_ESQUEMA_H
#define NL_FOUNDATION_XML_ESQUEMA_H

#include <nl/foundation/xml/erros_xsd.h>

#include <QString>

#include <expected>
#include <memory>

namespace nl::foundation::xml {

/// Esquema compilado e imutável. Copiar é barato: o handle libxml2 fica num recurso compartilhado.
class Esquema
{
public:
    /// Handle da libxml2 (xmlSchema). Definido só no .cpp; o cabeçalho público não expõe tipo do terceiro.
    class Recurso;

    /// Confere cada linha "<sha256>  <arquivo>" do manifesto (formato sha256sum) contra o arquivo em diretorio,
    /// exige que arquivoRaiz esteja listado e compila arquivoRaiz (os includes são resolvidos ao lado dele).
    static std::expected<Esquema, ErrosXsd> abrir(const QString& diretorio, const QString& manifesto,
                                                  const QString& arquivoRaiz);

    const Recurso& recurso() const noexcept;

private:
    explicit Esquema(std::shared_ptr<const Recurso> recurso) noexcept;

    std::shared_ptr<const Recurso> recurso_;
};

} // namespace nl::foundation::xml

#endif // NL_FOUNDATION_XML_ESQUEMA_H
