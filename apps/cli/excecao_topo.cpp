// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Exceção que chega ao topo da CLI (NL-2.9, ADR-0005 d). Ver excecao_topo.h. WIP: sem verificação.
#include "excecao_topo.h"

#include <nl/foundation/log/log.h>

#include <typeinfo>
#include <vector>

namespace nl::cli {

void registrarExcecaoNoTopo(nl::foundation::log::Log& registro, const std::exception& excecao, std::string_view posicao)
{
    const std::vector<nl::foundation::log::Campo> campos{
        nl::foundation::log::Campo{"tipo", typeid(excecao).name()},
        nl::foundation::log::Campo{"posicao", std::string(posicao)},
    };
    static_cast<void>(registro.registrar(nl::foundation::log::Nivel::Erro, "cli.excecao", campos));
}

} // namespace nl::cli
