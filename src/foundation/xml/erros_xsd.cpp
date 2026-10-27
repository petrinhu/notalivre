// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Erro da validação XSD (NL-1.6a). Ver erros_xsd.h.
#include <nl/foundation/xml/erros_xsd.h>

#include <optional>
#include <utility>

namespace nl::foundation::xml {

ErrosXsd::ErrosXsd(Motivo motivo, std::vector<nl::base::Erro> erros) : motivo_{motivo}, erros_{std::move(erros)}
{
    // Invariante: uma falha sempre traz ao menos um erro. Lista vazia seria uma falha sem explicação.
    if (erros_.empty()) {
        erros_.emplace_back("xsd.interno", std::nullopt, "falha sem detalhe da libxml2");
    }
}

} // namespace nl::foundation::xml
