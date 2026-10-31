// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Registro da CLI (NL-2.9, item 5, DA-2.53 e DA-2.54): a fábrica criarLogDaCli mora na raiz de composição, e a CLI só vê esta
// classe opaca, sem include de nl/foundation/log. A allowlist da CLI e a do cliente Sefin são montadas aqui, uma vez.
#ifndef NL_MID_COMPOSICAO_REGISTRO_CLI_H
#define NL_MID_COMPOSICAO_REGISTRO_CLI_H

#include <exception>
#include <memory>
#include <ostream>
#include <string_view>

namespace nl::foundation::log {
class Log;
}

namespace nl::mid::composicao {

class RegistroCli
{
public:
    explicit RegistroCli(std::ostream& destino);
    ~RegistroCli();
    RegistroCli(const RegistroCli&) = delete;
    RegistroCli& operator=(const RegistroCli&) = delete;

    /// Exceção que chega ao topo: o Log recebe o tipo e a posição, nunca o texto de what() (ADR-0005 d).
    void registrarExcecaoNoTopo(const std::exception& excecao, std::string_view posicao);

    /// Emissão aceita: o Id da DPS e a chave da NFS-e vão mascarados (allowlist do cliente Sefin, DA-2.54).
    void registrarEmissaoAceita(std::string_view idDps, std::string_view chaveAcesso);

    /// Emissão recusada: o Id da DPS vai mascarado e o código de recusa vai como está (DA-2.54).
    void registrarEmissaoRecusada(std::string_view idDps, std::string_view codigo);

    /// O Log que o cliente Sefin e o manipulador do Qt usam. Só a Mid chama; a CLI não.
    [[nodiscard]] nl::foundation::log::Log* logDoCliente() noexcept;

private:
    struct Interno;
    std::unique_ptr<Interno> interno_;
};

/// Fábrica da raiz de composição: o registro da CLI, com a allowlist e os eventos fechados. Usada pelo main e pelos testes.
std::unique_ptr<RegistroCli> criarLogDaCli(std::ostream& destino);

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_REGISTRO_CLI_H
