// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Aliases de texto e a identificação de tomador ou intermediário (NL-1.4, TCInfoPessoa, tiposComplexos_v1.01.xsd:1006).

#ifndef NL_BACK_DPS_IDENTIFICACAO_PESSOA_H
#define NL_BACK_DPS_IDENTIFICACAO_PESSOA_H

#include <variant>

#include <nl/back/dps/motivo_nao_nif.h>
#include <nl/back/dps/texto_limitado.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/cpf.h>


namespace nl::back {

/// Nome ou razão social, 1 a 300 caracteres (TSNomeRazaoSocial, tiposSimples_v1.01.xsd:417).
using NomeRazaoSocial = TextoLimitado<1, 300>;

/// Inscrição municipal, 1 a 15 caracteres (TSInscMun, tiposSimples_v1.01.xsd:410).
using InscricaoMunicipal = TextoLimitado<1, 15>;

/// E-mail, 1 a 80 caracteres (TSEmail, tiposSimples_v1.01.xsd:561).
using EmailPessoa = TextoLimitado<1, 80>;

/// NIF de pessoa no exterior, 1 a 40 caracteres (TSNIF, tiposSimples_v1.01.xsd:387).
using Nif = TextoLimitado<1, 40>;

/// Identificação de tomador ou intermediário: a choice de TCInfoPessoa (CNPJ, CPF, NIF ou cNaoNIF, tiposComplexos_v1.01.xsd:1006-1020).
using IdentificacaoPessoa = std::variant<Cnpj, Cpf, Nif, MotivoNaoNif>;

} // namespace nl::back

#endif // NL_BACK_DPS_IDENTIFICACAO_PESSOA_H
