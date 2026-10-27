// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código NBS do serviço (cNBS, 0-1), 9 dígitos. Só o formato; a tabela entra em NL-5.6. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1384 (TSCodNBS; elemento cNBS em tiposComplexos_v1.01.xsd:1483). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_NBS_H
#define NL_BACK_DPS_CODIGO_NBS_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código NBS do serviço (cNBS, 0-1), 9 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoNbs::de. O construtor é privado.
class CodigoNbs
{
public:
    /// Erro: "cnbs.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoNbs, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoNbs& outro) const = default;

private:
    explicit CodigoNbs(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_NBS_H
