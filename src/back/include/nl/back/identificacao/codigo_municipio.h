// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Código de município do IBGE, 7 dígitos (NL-1.1). Regra pura: sem I/O.

#ifndef NL_BACK_IDENTIFICACAO_CODIGO_MUNICIPIO_H
#define NL_BACK_IDENTIFICACAO_CODIGO_MUNICIPIO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Código de município com exatamente 7 dígitos. Guarda o texto, então o zero à esquerda não se perde.
/// Invariante: só se constrói por CodigoMunicipio::de. O construtor é privado.
class CodigoMunicipio
{
public:
    /// Erros: "municipio.tamanho" (diferente de 7 caracteres; não completa zero à esquerda),
    /// "municipio.formato" (caractere que não é dígito).
    static std::expected<CodigoMunicipio, nl::base::Erro> de(QStringView entrada);

    /// Os 7 dígitos.
    QString texto() const;

    bool operator==(const CodigoMunicipio& outro) const = default;

private:
    explicit CodigoMunicipio(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_IDENTIFICACAO_CODIGO_MUNICIPIO_H
