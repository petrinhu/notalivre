// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// cIndOp (código indicador da operação): 6 dígitos. Só o formato; a tabela entra em NL-5.6. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1716 (TSRTCCodIndOp). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_INDICADOR_OPERACAO_H
#define NL_BACK_DPS_CODIGO_INDICADOR_OPERACAO_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// cIndOp (código indicador da operação): 6 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoIndicadorOperacao::de. O construtor é privado.
class CodigoIndicadorOperacao
{
public:
    /// Erro: "cindop.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoIndicadorOperacao, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoIndicadorOperacao& outro) const = default;

private:
    explicit CodigoIndicadorOperacao(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_INDICADOR_OPERACAO_H
