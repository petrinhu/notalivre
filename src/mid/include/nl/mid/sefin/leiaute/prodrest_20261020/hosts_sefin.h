// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Hosts da API Sefin do leiaute prodrest_20261020 (NL-2.9, item 1, DA-2.5). A pasta do leiaute guarda os hosts; a política de
// ambiente compara o host exato, já analisado pelo QUrl, sem diferenciar maiúsculas. Nunca por sufixo nem por trecho.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_HOSTS_SEFIN_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_HOSTS_SEFIN_H

#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

/// Host da API de Produção (FATO: documentação oficial da Sefin Nacional, seção 5 da spec).
inline constexpr std::string_view kHostProducao = "sefin.nfse.gov.br";
/// Host da API de Produção Restrita (FATO: mesma documentação).
inline constexpr std::string_view kHostProducaoRestrita = "sefin.producaorestrita.nfse.gov.br";

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_HOSTS_SEFIN_H
