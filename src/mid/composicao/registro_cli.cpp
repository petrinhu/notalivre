// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Registro da CLI (NL-2.9, item 5, DA-2.53, DA-2.54). Ver registro_cli.h.
#include <nl/mid/composicao/registro_cli.h>

#include <nl/foundation/log/log.h>
#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>

#include <cctype>
#include <optional>
#include <typeinfo>
#include <utility>
#include <vector>

namespace nl::mid::composicao {
namespace fl = nl::foundation::log;

namespace {

// Código E no formato da API: E seguido de 3 ou 4 dígitos. Qualquer outro texto vira codigo_invalido (DA-2.54).
bool codigoEValido(std::string_view codigo)
{
    if (codigo.size() < 4 || codigo.size() > 5 || codigo.front() != 'E') {
        return false;
    }
    for (std::size_t i = 1; i < codigo.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(codigo[i])) == 0) {
            return false;
        }
    }
    return true;
}

// A allowlist da CLI: tipo e posição da exceção do topo, mais o que o cliente Sefin declara. Montada uma vez, aqui.
std::vector<fl::DeclaracaoCampo> allowlistDaCli()
{
    std::vector<fl::DeclaracaoCampo> campos{fl::DeclaracaoCampo{"tipo", fl::Regra::Cru}, fl::DeclaracaoCampo{"posicao", fl::Regra::Cru}};
    for (auto& campo : nl::mid::sefin::cliente::allowlistLogSefin()) {
        campos.push_back(std::move(campo));
    }
    return campos;
}

std::vector<std::string> eventosDaCli()
{
    std::vector<std::string> eventos{"cli.excecao", "qt.mensagem"};
    for (auto& evento : nl::mid::sefin::cliente::eventosLogSefin()) {
        eventos.push_back(std::move(evento));
    }
    return eventos;
}

} // namespace

struct RegistroCli::Interno
{
    explicit Interno(std::ostream& destino) : log(destino, "notalivre-cli", allowlistDaCli(), eventosDaCli()) {}
    fl::Log log;
};

RegistroCli::RegistroCli(std::ostream& destino) : interno_(std::make_unique<Interno>(destino)) {}

RegistroCli::~RegistroCli() = default;

void RegistroCli::registrarExcecaoNoTopo(const std::exception& excecao, std::string_view posicao)
{
    const std::vector<fl::Campo> campos{fl::Campo{"tipo", typeid(excecao).name()}, fl::Campo{"posicao", std::string(posicao)}};
    static_cast<void>(interno_->log.registrar(fl::Nivel::Erro, "cli.excecao", campos));
}

void RegistroCli::registrarEmissaoAceita(std::string_view idDps, std::string_view chaveAcesso)
{
    const std::vector<fl::Campo> campos{fl::Campo{"idDps", std::string(idDps)}, fl::Campo{"chaveAcesso", std::string(chaveAcesso)}};
    static_cast<void>(interno_->log.registrar(fl::Nivel::Info, "emissao.aceita", campos));
}

void RegistroCli::registrarEmissaoRecusada(std::string_view idDps, std::string_view codigo)
{
    const std::string codigoRegistrado = codigoEValido(codigo) ? std::string(codigo) : std::string("codigo_invalido");
    const std::vector<fl::Campo> campos{fl::Campo{"idDps", std::string(idDps)}, fl::Campo{"codigoE", codigoRegistrado}};
    static_cast<void>(interno_->log.registrar(fl::Nivel::Erro, "emissao.recusada", campos));
}

nl::foundation::log::Log* RegistroCli::logDoCliente() noexcept
{
    return &interno_->log;
}

std::unique_ptr<RegistroCli> criarLogDaCli(std::ostream& destino)
{
    return std::make_unique<RegistroCli>(destino);
}

} // namespace nl::mid::composicao
