// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Falhas do envio (NL-3.C, plano seção 3.3): como uma falha de transporte vira FalhaSefin com causa tipada. Funções puras,
// sem rede: o cliente Sefin chama no lugar do marcador de texto que decidia antes.
#ifndef NL_MID_SEFIN_CLIENTE_FALHAS_DO_ENVIO_H
#define NL_MID_SEFIN_CLIENTE_FALHAS_DO_ENVIO_H

#include <nl/back/ports/causa_falha_sefin.h>
#include <nl/back/ports/gateway_sefin.h>
#include <nl/foundation/transporte/transporte_https.h>

namespace nl::mid::sefin::cliente {

/// Causa que se sabe de uma falha de transporte: Tls sem codigosSsl é provável certificado do emissor; com codigosSsl, o servidor não foi confirmado.
[[nodiscard]] nl::back::CausaFalhaSefin causaDoTransporte(const nl::foundation::transporte::ErroTransporte& erro);

/// Falha de transporte com zero bytes escritos. No primeiro envio, Tls e configuração são definitivas; rede é transitória.
/// No reenvio, a Sefin pode já ter a DPS: vira transitória sem número (NaoGerada), nunca causa de recusa (R-10 b).
[[nodiscard]] nl::back::FalhaSefin falhaSemBytesEscritos(const nl::foundation::transporte::ErroTransporte& erro, bool reenvio);

/// Falha de rede na consulta GET /dps: tipo do transporte e causa, sem código E e sem mensagem.
[[nodiscard]] nl::back::FalhaSefin falhaDeRedeNaConsulta(const nl::foundation::transporte::ErroTransporte& erro);

/// Consulta que não disse o que aconteceu com o envio: Indeterminado, com a causa da consulta e sem código E.
[[nodiscard]] nl::back::FalhaSefin indeterminadoDepoisDaConsulta(const nl::back::FalhaSefin& consulta);

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_FALHAS_DO_ENVIO_H
