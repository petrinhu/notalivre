// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Endereço nacional: município (cMun) e CEP numérico de 8 dígitos (NL-1.4, TCEnderNac, tiposComplexos_v1.01.xsd:1189).

#ifndef NL_BACK_DPS_ENDERECO_NACIONAL_H
#define NL_BACK_DPS_ENDERECO_NACIONAL_H

#include <utility>

#include <nl/back/identificacao/cep.h>
#include <nl/back/identificacao/codigo_municipio.h>


namespace nl::back {

/// Invariante: os dois campos já vêm validados (CodigoMunicipio de 7 dígitos, Cep de 8 dígitos, NL-1.1).
class EnderecoNacional
{
public:
    static EnderecoNacional montar(CodigoMunicipio cMun, Cep cep)
    {
        return EnderecoNacional{std::move(cMun), std::move(cep)};
    }

    const CodigoMunicipio& cMun() const noexcept { return cMun_; }
    const Cep& cep() const noexcept { return cep_; }

    bool operator==(const EnderecoNacional& outro) const = default;

private:
    EnderecoNacional(CodigoMunicipio cMun, Cep cep) : cMun_{std::move(cMun)}, cep_{std::move(cep)} {}

    CodigoMunicipio cMun_;
    Cep cep_;
};

} // namespace nl::back

#endif // NL_BACK_DPS_ENDERECO_NACIONAL_H
