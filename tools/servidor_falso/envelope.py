# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Envelope da DPS no POST /nfse (DA-2.4). INFERÊNCIA até a NL-2.10: {"dpsXmlGZipB64": Base64(GZip RFC 1952)}.

GZip é o formato do gzip (cabeçalho 1f 8b), não zlib nem qCompress.
"""

import base64
import binascii
import gzip
import json
import zlib

CHAVE_ENVELOPE = "dpsXmlGZipB64"
MAGICA_GZIP = b"\x1f\x8b"


class ErroEnvelope(ValueError):
    """O corpo do POST não é um envelope de DPS válido."""


def gzip_b64(xml):
    return base64.b64encode(gzip.compress(xml, mtime=0)).decode("ascii")


def envelope(xml):
    return json.dumps({CHAVE_ENVELOPE: gzip_b64(xml)}).encode("utf-8")


def decodificar(corpo):
    try:
        doc = json.loads(corpo)
    except (ValueError, UnicodeDecodeError):
        raise ErroEnvelope("corpo não é JSON")
    if not isinstance(doc, dict) or CHAVE_ENVELOPE not in doc:
        raise ErroEnvelope(f"chave {CHAVE_ENVELOPE} ausente")
    try:
        bruto = base64.b64decode(doc[CHAVE_ENVELOPE], validate=True)
    except (binascii.Error, TypeError, ValueError):
        raise ErroEnvelope("Base64 inválido")
    if bruto[:2] != MAGICA_GZIP:
        raise ErroEnvelope("não é GZip RFC 1952 (cabeçalho 1f 8b)")
    try:
        return gzip.decompress(bruto)
    except (OSError, EOFError, zlib.error):
        raise ErroEnvelope("GZip corrompido")
