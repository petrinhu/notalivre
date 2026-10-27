// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Alíquota do ISSQN (pAliq, 0-1), padrão TSDec1V2: um dígito e até duas casas decimais. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1461 (TSDec1V2; elemento pAliq em tiposComplexos_v1.01.xsd:2034). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_ALIQUOTA_ISSQN_H
#define NL_BACK_DPS_ALIQUOTA_ISSQN_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Alíquota do ISSQN (pAliq, 0-1), padrão TSDec1V2: um dígito e até duas casas decimais. Invariante: o texto casa o padrão do XSD. Só se constrói por AliquotaIssqn::de. O construtor é privado.
class AliquotaIssqn
{
public:
    /// Erro: "aliquota_issqn.formato" (texto fora do padrão do XSD).
    static std::expected<AliquotaIssqn, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const AliquotaIssqn& outro) const = default;

private:
    explicit AliquotaIssqn(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_ALIQUOTA_ISSQN_H
