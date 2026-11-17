# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Veredito do portão de funções no CTest: faltas, sobras, exceções mortas e varredura de piso."""


def comparar(decl, reg, excecoes):
    """Linhas de falta, sobra e exceção morta dos programas analisados, com as contagens de falta e de sobra."""
    faltas, linhas_falta, linhas_sobra = set(), [], []
    for programa, declaradas in decl.items():
        registradas = reg.get(programa, [])
        for funcao in declaradas:
            if funcao not in registradas:
                chave = f"{programa}::{funcao}"
                faltas.add(chave)
                if chave not in excecoes:
                    linhas_falta.append(f"falta: {chave}")
        for funcao in registradas:
            if funcao not in declaradas:
                linhas_sobra.append(f"sobra: {programa}::{funcao}")
    mortas = [f"excecao morta: {chave}" for chave in excecoes if chave not in faltas]
    return linhas_falta + linhas_sobra + mortas, len(linhas_falta), len(linhas_sobra)


def mensagens_de_piso(cont):
    """Mensagem de varredura vazia ou incompleta, ou nenhuma."""
    if cont["ctest"] == 0 or cont["por_funcao"] == 0:
        return ["varredura vazia"]
    if cont["analisados"] != cont["encontrados"]:
        return [f"varredura incompleta: analisados={cont['analisados']} de encontrados={cont['encontrados']}"]
    return []
