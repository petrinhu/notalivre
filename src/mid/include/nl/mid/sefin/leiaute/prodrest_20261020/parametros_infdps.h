// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Parâmetros do infDPS no leiaute prodrest_20261020 (NL-2.8, adendo da DA-2.41; NL-2.9, DA-2.49). A pasta do leiaute decide os
// valores: a assinatura usa SHA-1 (DA-2.2), e o digest de comparação do E0014 usa SHA-256 (DA-2.41), e não se misturam. O motor
// xmldsig só recebe os parâmetros e não guarda constante própria. C14N, elemento, pai e namespace são comuns aos dois.
#ifndef NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H
#define NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H

#include <nl/foundation/xmldsig/assinador_xml.h>

#include <string_view>

namespace nl::mid::sefin::prodrest_20261020 {

inline constexpr std::string_view kUriC14nLeiaute = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
inline constexpr std::string_view kUriEnvelopedLeiaute = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";
inline constexpr std::string_view kUriSha1Leiaute = "http://www.w3.org/2000/09/xmldsig#sha1";
inline constexpr std::string_view kUriRsaSha1Leiaute = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
inline constexpr std::string_view kUriSha256Leiaute = "http://www.w3.org/2001/04/xmlenc#sha256";
inline constexpr std::string_view kUriRsaSha256Leiaute = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";
inline constexpr std::string_view kNamespaceLeiauteNfse = "http://www.sped.fazenda.gov.br/nfse";

/// Parâmetros da assinatura da DPS: SHA-1 e rsa-sha1 (DA-2.2). Se a NL-2.10 mostrar E0714 com SHA-1, troca-se só esta função.
inline nl::foundation::xmldsig::ParametrosAssinatura parametrosDaAssinaturaDps() {
    return {kUriC14nLeiaute, kUriEnvelopedLeiaute, kUriSha1Leiaute, kUriRsaSha1Leiaute, "infDPS", "DPS",
            kNamespaceLeiauteNfse, "NFSe", "infNFSe"};
}

/// Parâmetros do digest de comparação do E0014: SHA-256 (DA-2.41), só uso interno do comparador.
inline nl::foundation::xmldsig::ParametrosAssinatura parametrosDoDigestDeComparacao() {
    return {kUriC14nLeiaute, kUriEnvelopedLeiaute, kUriSha256Leiaute, kUriRsaSha256Leiaute, "infDPS", "DPS",
            kNamespaceLeiauteNfse, "NFSe", "infNFSe"};
}

} // namespace nl::mid::sefin::prodrest_20261020

#endif // NL_MID_SEFIN_LEIAUTE_PRODREST_20261020_PARAMETROS_INFDPS_H
