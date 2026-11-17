# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Caminho e leitura do fonte de cada programa do registro, com a escolha do leitor (Qt ou Python)."""

import os

from declaradas_py import casos_py
from declaradas_qt import slots_qt


def fora_da_raiz(programa):
    """Verdadeiro quando o nome do programa sai da raiz ou da pasta de build."""
    return programa.split(os.sep)[0] == ".."


def analisar_programa(raiz, programa, tipo):
    """Funções declaradas do programa, ou o motivo de não analisar: fora da raiz, inexistente ou fora do formato."""
    if fora_da_raiz(programa):
        return None, "caminho fora da raiz ou da pasta de build"
    caminho = os.path.join(raiz, programa)
    if not os.path.isfile(caminho):
        return None, "arquivo inexistente"
    try:
        with open(caminho, encoding="utf-8", errors="replace") as arq:
            texto = arq.read()
    except OSError:
        return None, "arquivo ilegivel"
    if tipo == "qt":
        return slots_qt(texto.splitlines())
    return casos_py(texto)


def declaradas_por_programa(raiz, por_programa, tipos):
    """Funções declaradas de cada programa do registro, e uma mensagem por programa que não se analisou."""
    decl, nao_analisados = {}, []
    for programa in sorted(por_programa):
        declaradas, motivo = analisar_programa(raiz, programa, tipos[programa])
        if motivo is not None:
            nao_analisados.append(f"nao analisado: {programa}: {motivo}")
        else:
            decl[programa] = declaradas
    return decl, nao_analisados
