// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Telefone do prestador, tomador ou destinatário: DDD mais número, de 6 a 20 dígitos. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:558 (TSTelefone). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_TELEFONE_H
#define NL_BACK_DPS_TELEFONE_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Telefone do prestador, tomador ou destinatário: DDD mais número, de 6 a 20 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por Telefone::de. O construtor é privado.
class Telefone
{
public:
    /// Erro: "telefone.formato" (texto fora do padrão do XSD).
    static std::expected<Telefone, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const Telefone& outro) const = default;

private:
    explicit Telefone(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_TELEFONE_H
