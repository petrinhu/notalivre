// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Chave de acesso da NFS-e, mínima para a numeração (NL-1.3): forma de 50 posições do TSIdNFSe sem o literal "NFS"
// (tiposSimples_v1.01.xsd:41, NFS[0-9]{9}[0-9A-Z]{14}[0-9]{27}). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CHAVE_ACESSO_H
#define NL_BACK_DPS_CHAVE_ACESSO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// Chave de acesso de 50 posições: nove dígitos, catorze de [0-9A-Z] e vinte e sete dígitos.
/// Invariante: só se constrói por ChaveAcesso::de. O construtor é privado.
class ChaveAcesso
{
public:
    /// Erros: "chave.tamanho" (diferente de 50 caracteres), "chave.formato" (posição fora do padrão).
    static std::expected<ChaveAcesso, nl::base::Erro> de(QStringView entrada);

    /// Os 50 caracteres.
    QString texto() const;

    bool operator==(const ChaveAcesso& outro) const = default;

private:
    explicit ChaveAcesso(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CHAVE_ACESSO_H
