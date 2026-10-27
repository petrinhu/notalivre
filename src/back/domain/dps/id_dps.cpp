// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Unidade de compilação do Id da DPS (NL-1.3). Montagem posicional: DPS + município + tipo + inscrição + série + número.
// CPF ocupa a inscrição com "000" à ESQUERDA e tipo 1; CNPJ (numérico ou alfanumérico) usa tipo 2 (onda1_leiaute.md, seção 3, item d).

#include <nl/back/dps/id_dps.h>

#include <utility>
#include <variant>

namespace nl::back {
namespace {

struct TipoEInscricao
{
    QChar tipo;
    QString inscricao;
};

TipoEInscricao campos(const Cpf& cpf)
{
    return TipoEInscricao{QLatin1Char('1'), QStringLiteral("000") + cpf.texto()};
}

TipoEInscricao campos(const Cnpj& cnpj)
{
    return TipoEInscricao{QLatin1Char('2'), cnpj.texto()};
}

} // namespace

std::expected<IdDps, nl::base::Erro> IdDps::montar(const CodigoMunicipio& municipio, const Inscricao& inscricao,
                                                   const Serie& serie, const NumeroDps& numero)
{
    const TipoEInscricao partes = std::visit([](const auto& valor) { return campos(valor); }, inscricao);
    return IdDps{QStringLiteral("DPS") + municipio.texto() + partes.tipo + partes.inscricao + serie.texto() + numero.texto()};
}

IdDps::IdDps(QString texto)
    : texto_{std::move(texto)}
{
}

QString IdDps::texto() const
{
    return texto_;
}

} // namespace nl::back
