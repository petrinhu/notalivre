// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Motivo do pulo de um número de DPS (NL-1.3). Regra pura: sem I/O.

#ifndef NL_BACK_NUMERACAO_MOTIVO_PULO_H
#define NL_BACK_NUMERACAO_MOTIVO_PULO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Motivo de pulo: texto não vazio e com pelo menos um caractere não branco.
/// Não tem construtor padrão: pular um número sem motivo não compila (plano, NL-1.3, puloSempreTemMotivo).
class MotivoPulo
{
public:
    /// Erro: "pulo.motivo_vazio" (vazio, ou só espaços e separadores).
    static std::expected<MotivoPulo, nl::base::Erro> de(QStringView entrada);

    QString texto() const;

    bool operator==(const MotivoPulo& outro) const = default;

private:
    explicit MotivoPulo(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_NUMERACAO_MOTIVO_PULO_H
