// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Série da DPS (NL-1.3): inteiro de 1 a 49999 escrito com cinco posições (TSIdDPS, tiposSimples_v1.01.xsd:54). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_SERIE_H
#define NL_BACK_DPS_SERIE_H

#include <expected>

#include <QString>

#include <nl/base/erro.h>

namespace nl::back {

/// Série da DPS. Invariante: valor entre 1 e 49999. Só se constrói por Serie::de. O construtor é privado.
class Serie
{
public:
    /// Erro: "serie.faixa" (fora de 1 a 49999).
    static std::expected<Serie, nl::base::Erro> de(int valor);

    /// As cinco posições, com zero à esquerda (por exemplo "00001").
    QString texto() const;

    bool operator==(const Serie& outro) const = default;

private:
    explicit Serie(int valor);

    int valor_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_SERIE_H
