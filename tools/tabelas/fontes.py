# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Âncora de confiança das fontes (NL-1.10): URL, tamanho e SHA-256 de cada Anexo, versionados em fontes.json.
# O conversor confere o SHA-256 dos bytes ANTES de abrir o XLSX (arquivo baixado é dado não confiável).
import hashlib
import json
from pathlib import Path


def carregar_fontes(caminho_json):
    """Lê fontes.json (versionado) e devolve o dicionário com os anexos A e B."""
    with open(caminho_json, encoding="utf-8") as arquivo:
        return json.load(arquivo)


def sha256_de_bytes(conteudo):
    return hashlib.sha256(conteudo).hexdigest()


def conferir_anexo(diretorio_xlsx, anexo):
    """Devolve (bytes_lidos, None) se o arquivo bate com a âncora; (None, motivo) se não bate ou falta."""
    caminho = Path(diretorio_xlsx) / anexo["arquivo"]
    if not caminho.is_file():
        return None, f"arquivo ausente: {caminho}"
    conteudo = caminho.read_bytes()
    if len(conteudo) != anexo["bytes"]:
        return None, f"{anexo['arquivo']}: {len(conteudo)} bytes, esperado {anexo['bytes']}"
    if sha256_de_bytes(conteudo) != anexo["sha256"]:
        return None, f"{anexo['arquivo']}: SHA-256 diverge da âncora em fontes.json"
    return conteudo, None
