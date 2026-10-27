// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Endereço no exterior: país, código postal, cidade e estado ou região (NL-1.4, TCEnderExt, tiposComplexos_v1.01.xsd:1204).

#ifndef NL_BACK_DPS_ENDERECO_EXTERIOR_H
#define NL_BACK_DPS_ENDERECO_EXTERIOR_H

#include <utility>

#include <nl/back/dps/codigo_pais.h>
#include <nl/back/dps/texto_limitado.h>


namespace nl::back {

/// Cidade, 1 a 60 caracteres (TSCidade, tiposSimples_v1.01.xsd:521).
using Cidade = TextoLimitado<1, 60>;

/// Estado, província ou região, 1 a 60 caracteres (TSEstadoProvRegiao, tiposSimples_v1.01.xsd:531).
using EstadoProvRegiao = TextoLimitado<1, 60>;

/// Código de endereçamento postal no exterior, de 1 a 11 caracteres (TSCodigoEndPostal, tiposSimples_v1.01.xsd:511-520).
/// Antes de 20/10 o XSD exigia 11 (linha Excel 714 do Anexo de 09/02, onda1_leiaute.md 2.3); o de 20/10 põe 1 a 11.
using CodigoEndPostal = TextoLimitado<1, 11>;

/// Invariante: os campos já vêm validados pelos próprios tipos.
class EnderecoExterior
{
public:
    static EnderecoExterior montar(CodigoPais cPais, CodigoEndPostal cEndPost, Cidade xCidade, EstadoProvRegiao xEstProvReg)
    {
        return EnderecoExterior{std::move(cPais), std::move(cEndPost), std::move(xCidade), std::move(xEstProvReg)};
    }

    const CodigoPais& cPais() const noexcept { return cPais_; }
    const CodigoEndPostal& cEndPost() const noexcept { return cEndPost_; }
    const Cidade& xCidade() const noexcept { return xCidade_; }
    const EstadoProvRegiao& xEstProvReg() const noexcept { return xEstProvReg_; }

    bool operator==(const EnderecoExterior& outro) const = default;

private:
    EnderecoExterior(CodigoPais cPais, CodigoEndPostal cEndPost, Cidade xCidade, EstadoProvRegiao xEstProvReg)
        : cPais_{std::move(cPais)}, cEndPost_{std::move(cEndPost)}, xCidade_{std::move(xCidade)}, xEstProvReg_{std::move(xEstProvReg)}
    {
    }

    CodigoPais cPais_;
    CodigoEndPostal cEndPost_;
    Cidade xCidade_;
    EstadoProvRegiao xEstProvReg_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_ENDERECO_EXTERIOR_H
