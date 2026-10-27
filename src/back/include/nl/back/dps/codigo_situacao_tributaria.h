// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// CST do IBS e da CBS: 3 dígitos. Só o formato; a tabela entra em NL-5.6. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1810 (TSRTCCodSitTrib). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_SITUACAO_TRIBUTARIA_H
#define NL_BACK_DPS_CODIGO_SITUACAO_TRIBUTARIA_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// CST do IBS e da CBS: 3 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoSituacaoTributaria::de. O construtor é privado.
class CodigoSituacaoTributaria
{
public:
    /// Erro: "cst.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoSituacaoTributaria, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoSituacaoTributaria& outro) const = default;

private:
    explicit CodigoSituacaoTributaria(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_SITUACAO_TRIBUTARIA_H
