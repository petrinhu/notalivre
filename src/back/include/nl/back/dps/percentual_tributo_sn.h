// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Percentual aproximado do Simples Nacional (pTotTribSN), padrão TSDec2V2. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1470 (TSDec2V2). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_PERCENTUAL_TRIBUTO_SN_H
#define NL_BACK_DPS_PERCENTUAL_TRIBUTO_SN_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Percentual aproximado do Simples Nacional (pTotTribSN), padrão TSDec2V2. Invariante: o texto casa o padrão do XSD. Só se constrói por PercentualTributoSn::de. O construtor é privado.
class PercentualTributoSn
{
public:
    /// Erro: "percentual_sn.formato" (texto fora do padrão do XSD).
    static std::expected<PercentualTributoSn, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const PercentualTributoSn& outro) const = default;

private:
    explicit PercentualTributoSn(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_PERCENTUAL_TRIBUTO_SN_H
