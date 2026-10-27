// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// CEP, 8 dígitos (NL-1.1, TSCEP). Regra pura: sem I/O.

#ifndef NL_BACK_IDENTIFICACAO_CEP_H
#define NL_BACK_IDENTIFICACAO_CEP_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// CEP com exatamente 8 dígitos. Aceita o hífen da máscara (01001-000) e guarda só os dígitos.
/// Invariante: só se constrói por Cep::de. O construtor é privado.
class Cep
{
public:
    /// Remove hífen e espaço (U+0020) e exige 8 dígitos.
    /// Erros: "cep.tamanho" (diferente de 8 caracteres; não completa zero à esquerda), "cep.formato" (não é dígito).
    static std::expected<Cep, nl::base::Erro> de(QStringView entrada);

    /// Os 8 dígitos, sem máscara.
    QString texto() const;

    bool operator==(const Cep& outro) const = default;

private:
    explicit Cep(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_IDENTIFICACAO_CEP_H
