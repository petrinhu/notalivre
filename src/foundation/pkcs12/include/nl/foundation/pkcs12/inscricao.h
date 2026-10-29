// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Inscrição do titular extraída do certificado (NL-2.2). Sai sempre como texto: o CNPJ pode ser alfanumérico.
#ifndef NL_FOUNDATION_PKCS12_INSCRICAO_H
#define NL_FOUNDATION_PKCS12_INSCRICAO_H

#include <QString>
#include <QStringView>

#include <optional>

namespace nl::foundation::pkcs12 {

enum class TipoInscricao {
    Cnpj,
    Cpf,
};

/// Identificador do titular. Invariante no construtor: só existe com formato válido.
class Inscricao {
public:
    /// CNPJ: 12 caracteres em [0-9A-Z] seguidos de 2 dígitos. nullopt se o formato não confere.
    [[nodiscard]] static std::optional<Inscricao> cnpj(QStringView texto);

    /// CPF: 11 dígitos. nullopt se o formato não confere.
    [[nodiscard]] static std::optional<Inscricao> cpf(QStringView texto);

    [[nodiscard]] TipoInscricao tipo() const noexcept { return tipo_; }
    [[nodiscard]] const QString& valor() const noexcept { return valor_; }

    friend bool operator==(const Inscricao& primeiro, const Inscricao& segundo) = default;

private:
    Inscricao(TipoInscricao tipo, QString valor);

    TipoInscricao tipo_;
    QString valor_;
};

/// Mesma raiz: os dois são CNPJ e os 8 primeiros caracteres são iguais, comparados como texto.
[[nodiscard]] bool mesmaRaizCnpj(const Inscricao& primeiro, const Inscricao& segundo);

} // namespace nl::foundation::pkcs12

#endif // NL_FOUNDATION_PKCS12_INSCRICAO_H
