// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de país ISO de duas letras maiúsculas (TSCodPaisISO). (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1362 (TSCodPaisISO). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_PAIS_H
#define NL_BACK_DPS_CODIGO_PAIS_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código de país ISO de duas letras maiúsculas (TSCodPaisISO). Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoPais::de. O construtor é privado.
class CodigoPais
{
public:
    /// Erro: "pais.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoPais, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoPais& outro) const = default;

private:
    explicit CodigoPais(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_PAIS_H
