// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Identificação fiscal, CPF (NL-1.1, DA-1.1). Regra pura: sem I/O.

#ifndef NL_BACK_IDENTIFICACAO_CPF_H
#define NL_BACK_IDENTIFICACAO_CPF_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// CPF de 11 dígitos, sem dígitos todos iguais, com dois dígitos verificadores válidos.
/// Invariante: só se constrói por Cpf::de. O construtor é privado.
class Cpf
{
public:
    /// Normaliza e valida. Remove ponto, hífen e espaço (U+0020) e exige 11 dígitos.
    /// Erros: "cpf.tamanho", "cpf.formato" (caractere que não é dígito), "cpf.repetido" (11 dígitos iguais), "cpf.dv".
    static std::expected<Cpf, nl::base::Erro> de(QStringView entrada);

    /// Os 11 dígitos, sem máscara.
    QString texto() const;

    /// Máscara XXX.XXX.XXX-DD.
    QString comMascara() const;

    bool operator==(const Cpf& outro) const = default;

private:
    explicit Cpf(QString normalizado);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_IDENTIFICACAO_CPF_H
