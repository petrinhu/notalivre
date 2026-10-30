// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.8): catálogo vazio. A tabela real entra no commit verde.
#include <nl/mid/sefin/cliente/catalogo_erros_sefin.h>

namespace nl::mid::sefin::cliente {

std::string_view CatalogoErrosSefin::mensagem(std::string_view) noexcept
{
    return {};
}

bool CatalogoErrosSefin::conhece(std::string_view) noexcept
{
    return false;
}

} // namespace nl::mid::sefin::cliente
