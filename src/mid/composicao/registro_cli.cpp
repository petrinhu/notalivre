// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Registro da CLI (NL-2.9, item 5). Ver registro_cli.h. Stub de teste: o corpo real vem no código.
#include <nl/mid/composicao/registro_cli.h>

namespace nl::mid::composicao {

struct RegistroCli::Interno
{
};

RegistroCli::RegistroCli(std::ostream& destino)
{
    static_cast<void>(destino);
}

RegistroCli::~RegistroCli() = default;

void RegistroCli::registrarExcecaoNoTopo(const std::exception& excecao, std::string_view posicao)
{
    static_cast<void>(excecao);
    static_cast<void>(posicao);
}

void RegistroCli::registrarEmissaoAceita(std::string_view idDps, std::string_view chaveAcesso)
{
    static_cast<void>(idDps);
    static_cast<void>(chaveAcesso);
}

void RegistroCli::registrarEmissaoRecusada(std::string_view idDps, std::string_view codigo)
{
    static_cast<void>(idDps);
    static_cast<void>(codigo);
}

nl::foundation::log::Log* RegistroCli::logDoCliente() noexcept
{
    return nullptr;
}

std::unique_ptr<RegistroCli> criarLogDaCli(std::ostream& destino)
{
    return std::make_unique<RegistroCli>(destino);
}

} // namespace nl::mid::composicao
