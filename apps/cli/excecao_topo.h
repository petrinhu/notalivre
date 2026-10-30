// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Exceção que chega ao topo da CLI (NL-2.9, ADR-0005 item d): o Log recebe só o tipo e a posição. A mensagem da exceção
// nunca entra no registro, porque pode carregar dado de titular.
#ifndef NL_CLI_EXCECAO_TOPO_H
#define NL_CLI_EXCECAO_TOPO_H

#include <exception>
#include <string_view>

namespace nl::foundation::log {
class Log;
}

namespace nl::cli {

/// Grava no registro o evento cli.excecao com os campos tipo e posicao. Sem o texto de what().
void registrarExcecaoNoTopo(nl::foundation::log::Log& registro, const std::exception& excecao, std::string_view posicao);

} // namespace nl::cli

#endif // NL_CLI_EXCECAO_TOPO_H
