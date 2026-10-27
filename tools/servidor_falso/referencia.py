# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""DPS de referência assinada no setup dos testes (DA-2.2 e DA-2.3). Não é o assinador do app (esse é a NL-2.3).

Assinatura irmã de infDPS dentro do elemento raiz DPS, rsa-sha1, digest sha1, C14N 20010315, enveloped, referência
ao Id do infDPS. Os URIs moram aqui só porque é o setup de teste: a NL-2.10 pode trocar por rsa-sha256 pela pasta
do leiaute.
"""

import re

URI_RSA_SHA1 = "http://www.w3.org/2000/09/xmldsig#rsa-sha1"
URI_SHA1 = "http://www.w3.org/2000/09/xmldsig#sha1"
URI_C14N = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315"
URI_ENVELOPED = "http://www.w3.org/2000/09/xmldsig#enveloped-signature"
ID_INFDPS = re.compile(rb'<infDPS Id="(DPS[0-9]{42})"')


def id_do_infdps(xml):
    m = ID_INFDPS.search(xml)
    if m is None:
        raise ValueError("infDPS sem Id no formato DPS + 42 dígitos")
    return m.group(1).decode("ascii")


def montar_template(xml, id_dps):
    """Insere o bloco Signature (sem DigestValue, SignatureValue nem X509) antes do último </DPS>."""
    assinatura = (
        '<Signature xmlns="http://www.w3.org/2000/09/xmldsig#"><SignedInfo>'
        f'<CanonicalizationMethod Algorithm="{URI_C14N}"/>'
        f'<SignatureMethod Algorithm="{URI_RSA_SHA1}"/>'
        f'<Reference URI="#{id_dps}"><Transforms>'
        f'<Transform Algorithm="{URI_ENVELOPED}"/>'
        f'<Transform Algorithm="{URI_C14N}"/></Transforms>'
        f'<DigestMethod Algorithm="{URI_SHA1}"/><DigestValue></DigestValue></Reference>'
        '</SignedInfo><SignatureValue></SignatureValue>'
        '<KeyInfo><X509Data></X509Data></KeyInfo></Signature>'
    ).encode("ascii")
    ancora = b"</DPS>"
    pos = xml.rfind(ancora)
    if pos < 0:
        raise ValueError("DPS sem </DPS>")
    return xml[:pos] + assinatura + xml[pos:]


def assinar(oraculo, xml_base, chave, certificado):
    """Devolve a DPS de referência assinada pelo xmlsec1 (oráculo de teste)."""
    return oraculo.assinar(montar_template(xml_base, id_do_infdps(xml_base)), chave, certificado)
