// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Percentual de tributo aproximado (pTotTribFed, pTotTribEst, pTotTribMun), padrão TSDec3V2. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1479 (TSDec3V2). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_PERCENTUAL_TRIBUTO_H
#define NL_BACK_DPS_PERCENTUAL_TRIBUTO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Percentual de tributo aproximado (pTotTribFed, pTotTribEst, pTotTribMun), padrão TSDec3V2. Invariante: o texto casa o padrão do XSD. Só se constrói por PercentualTributo::de. O construtor é privado.
class PercentualTributo
{
public:
    /// Erro: "percentual.formato" (texto fora do padrão do XSD).
    static std::expected<PercentualTributo, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const PercentualTributo& outro) const = default;

private:
    explicit PercentualTributo(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_PERCENTUAL_TRIBUTO_H
