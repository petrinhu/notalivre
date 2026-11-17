# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Lista de exceções do portão de funções no CTest: uma linha por programa::função, com motivo."""

from parada import Parada


def ler_excecoes(caminho):
    """Lê a lista programa::funcao | motivo; linha sem motivo, sem formato ou repetida é Parada."""
    try:
        with open(caminho, encoding="utf-8") as arq:
            linhas = arq.read().splitlines()
    except (OSError, ValueError):
        raise Parada(f"lista de excecoes ilegivel: {caminho}")
    excecoes = {}
    for n, linha in enumerate(linhas, 1):
        limpa = linha.strip()
        if not limpa or limpa.startswith("#"):
            continue
        if "|" not in limpa or "::" not in limpa.split("|", 1)[0]:
            raise Parada(f"formato invalido na linha {n}")
        chave, motivo = (parte.strip() for parte in limpa.split("|", 1))
        if not motivo:
            raise Parada(f"linha sem motivo: {n}")
        if chave in excecoes:
            raise Parada(f"linha repetida: {chave}")
        excecoes[chave] = motivo
    return excecoes
