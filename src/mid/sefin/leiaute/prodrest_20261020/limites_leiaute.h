// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Valores do leiaute de Produção Restrita de 20/10/2026 (NL-1.6b, DA-1.2). Só esta pasta conhece estes números:
// o Back e o QML recebem o limite pela fachada, nunca por constante própria.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_LIMITES_LEIAUTE_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_LIMITES_LEIAUTE_H

#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

/// Atributo versao do TCDPS (TVerNFSe, tiposSimples_v1.01.xsd:3-7: 1.00 ou 1.01).
inline constexpr std::string_view kVersaoLeiaute = "1.01";

/// Tamanho máximo de xDescServ (TSDesc2000, tiposSimples_v1.01.xsd:1439-1445), em pontos de código Unicode.
inline constexpr int kLimiteXDescServ = 2000;

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_LIMITES_LEIAUTE_H
