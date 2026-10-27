// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// cClassTrib do IBS e da CBS: 6 dígitos. Só o formato; a tabela entra em NL-5.6. (NL-1.4). Padrão: tiposSimples_v1.01.xsd:1821 (TSRTCCodClassTrib). Regra pura: sem I/O.

#ifndef NL_BACK_DPS_CODIGO_CLASSIFICACAO_TRIBUTARIA_H
#define NL_BACK_DPS_CODIGO_CLASSIFICACAO_TRIBUTARIA_H

#include <expected>

#include <QString>
#include <QStringView>

#include <nl/base/erro.h>

namespace nl::back {

/// cClassTrib do IBS e da CBS: 6 dígitos. Invariante: o texto casa o padrão do XSD. Só se constrói por CodigoClassificacaoTributaria::de. O construtor é privado.
class CodigoClassificacaoTributaria
{
public:
    /// Erro: "cclasstrib.formato" (texto fora do padrão do XSD).
    static std::expected<CodigoClassificacaoTributaria, nl::base::Erro> de(QStringView entrada);

    /// Texto como veio, já validado.
    QString texto() const;

    bool operator==(const CodigoClassificacaoTributaria& outro) const = default;

private:
    explicit CodigoClassificacaoTributaria(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_CODIGO_CLASSIFICACAO_TRIBUTARIA_H
