// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do motivo de pulo (NL-1.3): texto com pelo menos um caractere não branco.

#include <nl/back/numeracao/motivo_pulo.h>

#include <optional>
#include <utility>

namespace nl::back {

std::expected<MotivoPulo, nl::base::Erro> MotivoPulo::de(QStringView entrada)
{
    if (entrada.trimmed().isEmpty()) {
        return std::unexpected(nl::base::Erro{"pulo.motivo_vazio", std::nullopt, "motivo do pulo não pode ser vazio"});
    }
    return MotivoPulo{entrada.toString()};
}

MotivoPulo::MotivoPulo(QString texto)
    : texto_{std::move(texto)}
{
}

QString MotivoPulo::texto() const
{
    return texto_;
}

} // namespace nl::back
