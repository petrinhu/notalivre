// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Identificação fiscal, CNPJ numérico e alfanumérico (NL-1.1, DA-1.1). Regra pura: sem I/O.

#ifndef NL_BACK_IDENTIFICACAO_CNPJ_H
#define NL_BACK_IDENTIFICACAO_CNPJ_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// CNPJ de 14 caracteres [0-9A-Z] com dois dígitos verificadores válidos.
/// Invariante: só se constrói por Cnpj::de, que normaliza e valida. O construtor é privado.
class Cnpj
{
public:
    /// Normaliza (sem validar): remove ponto, barra, hífen e espaço (U+0020) e converte a-z em A-Z (só ASCII).
    static QString normalizar(QStringView entrada);

    /// Normaliza e valida. Erros: "cnpj.formato" (caractere fora de [0-9A-Z], ou posição 13 ou 14 não numérica),
    /// "cnpj.tamanho" (diferente de 14 caracteres depois da normalização; não completa zero à esquerda),
    /// "cnpj.dv" (dígito verificador não confere), "cnpj.zerado" (só 00000000000000, DA-1.9).
    static std::expected<Cnpj, nl::base::Erro> de(QStringView entrada);

    /// Os 14 caracteres em maiúsculas, sem máscara.
    QString texto() const;

    /// As 8 primeiras posições (raiz).
    QString raiz() const;

    /// Máscara oficial XX.XXX.XXX/XXXX-DD (Q.21 da Receita Federal).
    QString comMascara() const;

    bool operator==(const Cnpj& outro) const = default;

private:
    explicit Cnpj(QString normalizado);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_IDENTIFICACAO_CNPJ_H
