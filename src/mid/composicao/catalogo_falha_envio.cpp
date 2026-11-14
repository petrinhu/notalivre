// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Catálogo da falha de envio (NL-3.C, plano seção 3.4). Ver catalogo_falha_envio.h. Corpos provisórios: os textos entram no G1.
#include <nl/mid/composicao/catalogo_falha_envio.h>

namespace nl::mid::composicao {

TextoDaFalha textoDaClasse(ClasseFalhaEnvio /*classe*/) noexcept
{
    return TextoDaFalha{"", ""};
}

bool ehFalhaDeEnvio(std::string_view /*codigo*/) noexcept
{
    return false;
}

} // namespace nl::mid::composicao
