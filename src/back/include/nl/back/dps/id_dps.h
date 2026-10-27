// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Id da DPS de 45 posições (NL-1.3): "DPS" + Cód.Mun.(7) + tipo(1) + inscrição(14) + série(5) + número(15).
// Fontes: tiposSimples_v1.01.xsd:44-54 (TSIdDPS) e docs/pesquisa/onda1_leiaute.md, seção 3, item (d).
// Regra pura: sem I/O.

#ifndef NL_BACK_DPS_ID_DPS_H
#define NL_BACK_DPS_ID_DPS_H

#include <expected>

#include <QString>

#include <nl/base/erro.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/inscricao.h>

namespace nl::back {

/// Id da DPS. Invariante: 45 posições, o padrão do XSD, e o CPF sempre com "000" à ESQUERDA na inscrição
/// (tipo 1). CNPJ, numérico ou alfanumérico, usa tipo 2 e ocupa as 14 posições sem preenchimento.
/// Só se constrói por IdDps::montar. O construtor é privado.
class IdDps
{
public:
    /// Monta o Id a partir de tipos já validados. A validação está nos tipos de entrada, então a montagem não falha
    /// hoje; o retorno expected fica no contrato para que uma regra futura de montagem não precise mudar a assinatura.
    static std::expected<IdDps, nl::base::Erro> montar(const CodigoMunicipio& municipio, const Inscricao& inscricao,
                                                       const Serie& serie, const NumeroDps& numero);

    /// Os 45 caracteres do Id, sem espaço.
    QString texto() const;

    bool operator==(const IdDps& outro) const = default;

private:
    explicit IdDps(QString texto);

    QString texto_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_ID_DPS_H
