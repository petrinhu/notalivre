// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de tributação nacional (cTribNac), subitem da lista nacional de serviços, 6 dígitos. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1373 (TSCodTribNac). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_TRIB_NACIONAL_H
#define NL_BACK_DPS_CODIGO_TRIB_NACIONAL_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código de tributação nacional (cTribNac), subitem da lista nacional de serviços, 6 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoTribNacional::de. O construtor é privado.
class CodigoTribNacional
{
public:
    /// Erro: "cod_trib_nacional.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoTribNacional, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoTribNacional& outro) const = default;

private:
    explicit CodigoTribNacional(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_TRIB_NACIONAL_H
