// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Stub do bloco M-R (NL-3.1a): a política entra no M-G. Devolve persistencia.nao_implementado para qualquer entrada.

#include "nl/mid/persistencia/politica_conexao.h"

#include <optional>
#include <utility>

namespace nl::mid::persistencia {

std::expected<nl::foundation::sql::DadosConexao, nl::base::Erro>
dadosDeConexao(const ServidorDeclarado&, const nl::foundation::pkcs12::SenhaSegura&)
{
    return std::unexpected(nl::base::Erro("persistencia.nao_implementado", std::nullopt, "não implementado"));
}

} // namespace nl::mid::persistencia
