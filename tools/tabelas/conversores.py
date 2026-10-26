# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Conversão das quatro tabelas da NL-1.10 a partir das abas já lidas. Toda divergência vira erro listado
# (linha e motivo); nenhuma linha é descartada em silêncio. Quem chama decide se a saída é gravada.
import unicodedata
from collections import Counter

import normaliza

ABA_SERVICO = "LISTA.SERV.NAC."
ABA_NBS = "LISTA.NBS_v2.0"
ABA_MUNICIPIOS = "TAB.MUN_IBGE"
ABA_PAISES = "TAB.PAÍS_ISO2"
ABA_LOCALIDADES = "TAB.LOC.GERAL"

CABECALHO_SERVICO = {"A": "CÓDIGO DE TRIBUTAÇÃO NACIONAL", "B": "ITEM", "C": "SUBITEM",
                     "D": "DESDOBRO NACIONAL", "E": "DESCRIÇÃO"}
CABECALHO_NBS = {"A": "CÓDIGO NBS", "B": "DESCRIÇÃO"}
CABECALHO_MUNICIPIOS = {"A": "NOME UF", "B": "Sigla UF", "C": "NOME MUNICÍPIO", "D": "CÓDIGO COMPLETO"}
CABECALHO_PAISES = {"A": "PAÍS ISO 2", "B": "NOME PAÍS"}
CABECALHO_LOCALIDADES = {"A": "LOCALIDADE DE \nPRESTAÇÃO DE SERVIÇO\nGERAL", "B": "CÓDIGO COMPLETO"}


class Resultado:
    """Registros convertidos, contagens e erros de uma tabela."""

    def __init__(self, nome):
        self.nome = nome
        self.registros = []
        self.contagens = Counter()
        self.erros = []

    def erro(self, linha, motivo):
        self.erros.append(f"{self.nome} linha {linha}: {motivo}")


def _texto(celulas, coluna):
    valor = celulas.get(coluna)
    return valor.strip() if valor is not None else ""


def _cabecalho_confere(linhas, esperado, resultado):
    if not linhas or linhas[0][0] != 1:
        resultado.erro(1, "aba sem cabeçalho na linha 1")
        return
    celulas = linhas[0][1]
    for coluna, titulo in esperado.items():
        if _texto(celulas, coluna) != titulo:
            resultado.erro(1, f"cabeçalho da coluna {coluna} diverge: {_texto(celulas, coluna)!r}")


def _vazia(celulas):
    return all(valor is None or str(valor).strip() == "" for valor in celulas.values())


def _dados(linhas, esperado, resultado):
    """Linhas de dados (a partir da 2) com cabeçalho conferido.

    Linha vazia depois do último dado é contada em vazias_finais (a TAB.LOC.GERAL traz uma). Linha vazia no meio da
    aba é erro: não há dado para perder em silêncio, nem razão para aceitar um buraco.
    """
    _cabecalho_confere(linhas, esperado, resultado)
    corpo = linhas[1:]
    ultimo = max((indice for indice, (_, celulas) in enumerate(corpo) if not _vazia(celulas)), default=-1)
    dados = []
    for indice, (numero, celulas) in enumerate(corpo):
        if not _vazia(celulas):
            dados.append((numero, celulas))
        elif indice > ultimo:
            resultado.contagens["vazias_finais"] += 1
        else:
            resultado.erro(numero, "linha vazia dentro da aba")
    resultado.contagens["linhas"] = len(dados)
    return dados


def _inteiro_de_texto(texto):
    if texto is None or not texto.isascii() or not texto.isdigit():
        return None
    return int(texto)


def converter_servico(linhas):
    resultado = Resultado("servico")
    for numero, celulas in _dados(linhas, CABECALHO_SERVICO, resultado):
        item = _inteiro_de_texto(_texto(celulas, "B"))
        subitem = _inteiro_de_texto(_texto(celulas, "C"))
        desdobro = _inteiro_de_texto(_texto(celulas, "D"))
        descricao = _texto(celulas, "E")
        if item is None or subitem is None or desdobro is None:
            resultado.erro(numero, "item, subitem ou desdobro não é inteiro")
            continue
        codigo_bruto = _texto(celulas, "A")
        if not codigo_bruto:
            # Linha de agrupamento (item ou subitem): só vale com desdobro zero.
            if desdobro != 0:
                resultado.erro(numero, "linha sem cTribNac com desdobro diferente de zero")
            resultado.contagens["sem_codigo"] += 1
            continue
        try:
            codigo = normaliza.ctribnac_texto(codigo_bruto)
        except ValueError as motivo:
            resultado.erro(numero, str(motivo))
            continue
        if desdobro == 0:
            resultado.erro(numero, f"cTribNac {codigo} com desdobro zero")
            continue
        if not normaliza.ctribnac_confere(codigo, item, subitem, desdobro):
            resultado.erro(numero, f"cTribNac {codigo} não bate com item {item}, subitem {subitem}, desdobro {desdobro}")
            continue
        if not descricao:
            resultado.erro(numero, f"cTribNac {codigo} sem descrição")
            continue
        if any(registro["codigo"] == codigo for registro in resultado.registros):
            resultado.erro(numero, f"cTribNac {codigo} duplicado")
            continue
        resultado.contagens["com_codigo"] += 1
        resultado.contagens["codigo_6_digitos" if len(codigo_bruto) == 6 else "codigo_5_digitos"] += 1
        if normaliza.grupo_sem_issqn(codigo):
            resultado.contagens["grupo_99"] += 1
        resultado.registros.append({
            "codigo": codigo,
            "item": f"{item:02d}",
            "subitem": f"{subitem:02d}",
            "desdobro": f"{desdobro:02d}",
            "descricao": descricao,
        })
    return resultado


def converter_nbs(linhas):
    resultado = Resultado("nbs")
    vistos = set()
    for numero, celulas in _dados(linhas, CABECALHO_NBS, resultado):
        try:
            codigo = normaliza.nbs_texto(celulas.get("A"))
        except ValueError as motivo:
            resultado.erro(numero, str(motivo))
            continue
        descricao = _texto(celulas, "B")
        if codigo in vistos:
            resultado.erro(numero, f"NBS {codigo} duplicado")
            continue
        vistos.add(codigo)
        # Códigos sentinela (99999, 9.9999.99.99) vêm sem texto na fonte: ficam, com descrição vazia, e são contados.
        if not descricao:
            resultado.contagens["sem_descricao"] += 1
        resultado.contagens["pontuados" if "." in codigo else "sem_pontos"] += 1
        resultado.registros.append({"codigo": codigo, "descricao": descricao})
    return resultado


def converter_municipios(linhas):
    resultado = Resultado("municipios")
    vistos = set()
    for numero, celulas in _dados(linhas, CABECALHO_MUNICIPIOS, resultado):
        try:
            codigo = normaliza.codigo_municipio(_texto(celulas, "D"))
            sigla, nome_uf = normaliza.uf_do_municipio(codigo)
        except ValueError as motivo:
            resultado.erro(numero, str(motivo))
            continue
        nome_uf_planilha = unicodedata.normalize("NFC", _texto(celulas, "A"))
        if nome_uf_planilha != unicodedata.normalize("NFC", nome_uf):
            resultado.erro(numero, f"NOME UF {nome_uf_planilha!r} não bate com o código {codigo}")
            continue
        sigla_planilha = _texto(celulas, "B")
        if sigla_planilha:
            if sigla_planilha != sigla:
                resultado.erro(numero, f"Sigla UF {sigla_planilha!r} não bate com o código {codigo}")
                continue
            resultado.contagens["siglas_planilha"] += 1
        else:
            resultado.contagens["siglas_derivadas"] += 1
        nome = _texto(celulas, "C")
        if not nome:
            resultado.erro(numero, f"município {codigo} sem nome")
            continue
        if codigo in vistos:
            resultado.erro(numero, f"município {codigo} duplicado")
            continue
        vistos.add(codigo)
        resultado.registros.append({"codigo": codigo, "nome": nome, "uf": sigla})
    return resultado


def converter_paises(linhas):
    resultado = Resultado("paises")
    vistos = set()
    for numero, celulas in _dados(linhas, CABECALHO_PAISES, resultado):
        iso2 = _texto(celulas, "A")
        nome = _texto(celulas, "B")
        if len(iso2) != 2 or not iso2.isascii() or not iso2.isalpha() or not iso2.isupper():
            resultado.erro(numero, f"código ISO2 inválido: {iso2!r}")
            continue
        if not nome:
            resultado.erro(numero, f"país {iso2} sem nome")
            continue
        if iso2 in vistos:
            resultado.erro(numero, f"país {iso2} duplicado")
            continue
        vistos.add(iso2)
        resultado.registros.append({"iso2": iso2, "nome": nome})
    return resultado


def converter_localidades(linhas):
    resultado = Resultado("localidades")
    vistos = set()
    for numero, celulas in _dados(linhas, CABECALHO_LOCALIDADES, resultado):
        try:
            codigo = normaliza.codigo_localidade(_texto(celulas, "B"))
        except ValueError as motivo:
            resultado.erro(numero, str(motivo))
            continue
        nome = _texto(celulas, "A")
        if not nome:
            resultado.erro(numero, f"localidade {codigo} sem nome")
            continue
        if codigo in vistos:
            resultado.erro(numero, f"localidade {codigo} duplicada")
            continue
        vistos.add(codigo)
        resultado.registros.append({"codigo": codigo, "nome": nome})
    return resultado
