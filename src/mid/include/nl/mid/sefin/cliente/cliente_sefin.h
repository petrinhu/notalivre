// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// ClienteSefin (NL-2.8): implementação do port GatewaySefin sobre o TransporteHttps da NL-2.5, com o envelope da NL-2.4
// e a política de retry. Cabeçalho público do Mid: só declarações antecipadas (transporte e registro ficam na Foundation).
#ifndef NL_MID_SEFIN_CLIENTE_CLIENTE_SEFIN_H
#define NL_MID_SEFIN_CLIENTE_CLIENTE_SEFIN_H

#include <nl/back/ports/gateway_sefin.h>
#include <nl/mid/sefin/cliente/comparador_infdps.h>
#include <nl/mid/sefin/cliente/politica_retry.h>

namespace nl::foundation::transporte {
class TransporteHttps;
}

namespace nl::foundation::log {
class Log;
}

namespace nl::mid::sefin::cliente {

class ClienteSefin final : public nl::back::GatewaySefin {
public:
    /// transporte, registro e comparador são emprestados: o chamador mantém os três vivos. Nenhum dado de titular vai
    /// ao registro. Sem comparador real, o padrão faz a reconciliação de E0014 terminar em Indeterminado.
    ClienteSefin(nl::foundation::transporte::TransporteHttps& transporte, nl::foundation::log::Log& registro,
                 PoliticaRetry politica, const ComparadorInfDps& comparador = comparadorIndisponivel());

    [[nodiscard]] std::expected<nl::back::NotaSefin, nl::back::FalhaSefin> emitir(
        const nl::back::DpsAssinada& dps) override;
    [[nodiscard]] std::expected<nl::back::NotaSefin, nl::back::FalhaSefin> consultarNota(
        const nl::back::ChaveAcesso& chave) override;
    [[nodiscard]] std::expected<nl::back::ChaveAcesso, nl::back::FalhaSefin> consultarDps(
        const nl::back::IdDps& id) override;

private:
    nl::foundation::transporte::TransporteHttps& transporte_;
    nl::foundation::log::Log& registro_;
    PoliticaRetry politica_;
    const ComparadorInfDps& comparador_;
};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_CLIENTE_SEFIN_H
