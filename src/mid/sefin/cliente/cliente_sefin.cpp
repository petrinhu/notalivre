// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ESQUELETO VERMELHO (NL-2.8): toda chamada recusa com erro definitivo. O cliente real entra no commit verde.
#include <nl/mid/sefin/cliente/cliente_sefin.h>

namespace nl::mid::sefin::cliente {

ClienteSefin::ClienteSefin(nl::foundation::transporte::TransporteHttps& transporte, nl::foundation::log::Log& registro,
                           PoliticaRetry politica)
    : transporte_(transporte), registro_(registro), politica_(politica)
{
}

std::expected<nl::back::NotaSefin, nl::back::FalhaSefin> ClienteSefin::emitir(const nl::back::DpsAssinada&)
{
    return std::unexpected(nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Definitiva, "", "esqueleto"});
}

std::expected<nl::back::NotaSefin, nl::back::FalhaSefin> ClienteSefin::consultarNota(const nl::back::ChaveAcesso&)
{
    return std::unexpected(nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Definitiva, "", "esqueleto"});
}

std::expected<nl::back::ChaveAcesso, nl::back::FalhaSefin> ClienteSefin::consultarDps(const nl::back::IdDps&)
{
    return std::unexpected(nl::back::FalhaSefin{nl::back::MotivoFalhaSefin::Definitiva, "", "esqueleto"});
}

} // namespace nl::mid::sefin::cliente
