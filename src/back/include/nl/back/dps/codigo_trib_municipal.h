// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de tributação municipal (cTribMun, 0-1), 3 dígitos (NL-2.10h, H1). Padrão: tiposSimples_v1.01.xsd:568 (TCCodTribMun). 000 é recusado pela regra E0315 (Anexo I, linha Excel 342). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_TRIB_MUNICIPAL_H
#define NL_BACK_DPS_CODIGO_TRIB_MUNICIPAL_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código de tributação municipal (cTribMun), 3 dígitos. Invariante: o texto casa [0-9]{3} e não é 000. Só se constrói por CodigoTribMunicipal::de. O construtor é privado.
class CodigoTribMunicipal
{
public:
    /// Erros: "cod_trib_municipal.formato" (fora de 3 dígitos) e "cod_trib_municipal.zero" (000, regra E0315).
    static std::expected<CodigoTribMunicipal, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoTribMunicipal& outro) const = default;

private:
    explicit CodigoTribMunicipal(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_TRIB_MUNICIPAL_H
