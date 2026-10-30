// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parâmetros do infDPS no leiaute prodrest_20261020 (NL-2.8, adendo da DA-2.41): C14N 1.0, transformada enveloped e
// SHA-256, com o Id no infDPS filho da DPS, no namespace do leiaute. A pasta do leiaute decide esses valores; o motor
// xmldsig só recebe os parâmetros e não guarda constante própria (DA-2.2, DA-2.40 item 2).
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H

#include <nl/foundation/xmldsig/assinador_xml.h>

#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

inline constexpr std::string_view kUriC14nLeiaute = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
inline constexpr std::string_view kUriEnvelopedLeiaute = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
inline constexpr std::string_view kUriSha256Leiaute = "http://www.w3.org/2001/04/xmlenc#sha256";
inline constexpr std::string_view kNamespaceLeiauteNfse = "http://www.sped.fazenda.gov.br/nfse";

/// Parâmetros para o digest canônico do infDPS: referência "infDPS" filha de "DPS", no namespace do leiaute.
inline nl::foundation::xmldsig::ParametrosAssinatura parametrosDoInfDps() {
    // Rodada 5 (revisão): a NFS-e no leiaute traz NFSe/infNFSe/DPS/infDPS, e o digest aceita essa forma pelo caminho fixo.
    return {kUriC14nLeiaute, kUriEnvelopedLeiaute, kUriSha256Leiaute, std::string_view{}, "infDPS", "DPS",
            kNamespaceLeiauteNfse, "NFSe", "infNFSe"};
}

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H
