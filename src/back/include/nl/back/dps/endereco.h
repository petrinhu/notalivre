// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Endereço completo: local nacional ou no exterior mais logradouro, número, complemento e bairro (NL-1.4, TCEndereco, tiposComplexos_v1.01.xsd:1075).

#ifndef NL_BACK_DPS_ENDERECO_H
#define NL_BACK_DPS_ENDERECO_H

#include <optional>
#include <utility>
#include <variant>

#include <nl/back/dps/endereco_exterior.h>
#include <nl/back/dps/endereco_nacional.h>
#include <nl/back/dps/texto_limitado.h>


namespace nl::back {

/// Logradouro, 1 a 255 caracteres (TSLogradouro, tiposSimples_v1.01.xsd:431).
using Logradouro = TextoLimitado<1, 255>;

/// Número do endereço, 1 a 60 caracteres (TSNumeroEndereco, tiposSimples_v1.01.xsd:440).
using NumeroEndereco = TextoLimitado<1, 60>;

/// Complemento, 1 a 156 caracteres (TSComplementoEndereco, tiposSimples_v1.01.xsd:449).
using ComplementoEndereco = TextoLimitado<1, 156>;

/// Bairro, 1 a 60 caracteres (TSBairro, tiposSimples_v1.01.xsd:458).
using Bairro = TextoLimitado<1, 60>;

/// Escolha de TCEndereco: endNac ou endExt (tiposComplexos_v1.01.xsd:1078-1083).
using LocalEndereco = std::variant<EnderecoNacional, EnderecoExterior>;

/// Invariante: os campos já vêm validados pelos próprios tipos.
class Endereco
{
public:
    static Endereco montar(LocalEndereco local, Logradouro xLgr, NumeroEndereco nro, std::optional<ComplementoEndereco> xCpl, Bairro xBairro)
    {
        return Endereco{std::move(local), std::move(xLgr), std::move(nro), std::move(xCpl), std::move(xBairro)};
    }

    const LocalEndereco& local() const noexcept { return local_; }
    const Logradouro& xLgr() const noexcept { return xLgr_; }
    const NumeroEndereco& nro() const noexcept { return nro_; }
    const std::optional<ComplementoEndereco>& xCpl() const noexcept { return xCpl_; }
    const Bairro& xBairro() const noexcept { return xBairro_; }

    bool operator==(const Endereco& outro) const = default;

private:
    Endereco(LocalEndereco local, Logradouro xLgr, NumeroEndereco nro, std::optional<ComplementoEndereco> xCpl, Bairro xBairro)
        : local_{std::move(local)}, xLgr_{std::move(xLgr)}, nro_{std::move(nro)}, xCpl_{std::move(xCpl)}, xBairro_{std::move(xBairro)}
    {
    }

    LocalEndereco local_;
    Logradouro xLgr_;
    NumeroEndereco nro_;
    std::optional<ComplementoEndereco> xCpl_;
    Bairro xBairro_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_ENDERECO_H
