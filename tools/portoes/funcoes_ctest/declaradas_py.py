# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Métodos de caso de teste de um script Python, lidos pelo ast sem importar o módulo."""

import ast

EXCLUIDAS_PY = {"setUp", "tearDown", "setUpClass", "tearDownClass"}


def excluida_py(nome):
    """Verdadeiro para os métodos Python que não são caso de teste (ciclo de vida e nome com _)."""
    return nome.startswith("_") or nome in EXCLUIDAS_PY


def _e_testcase(base):
    """Verdadeiro quando a base da classe é TestCase, por nome simples ou por atributo."""
    return (isinstance(base, ast.Name) and base.id == "TestCase") or (
        isinstance(base, ast.Attribute) and base.attr == "TestCase"
    )


def casos_py(texto):
    """Métodos das classes TestCase do topo do módulo, sem os de ciclo de vida e os de nome com _, ou o motivo."""
    try:
        arvore = ast.parse(texto)
    except SyntaxError as erro:
        return None, f"erro de sintaxe na linha {erro.lineno}"
    metodos, achou = [], False
    for classe in arvore.body:
        if not isinstance(classe, ast.ClassDef) or not any(_e_testcase(b) for b in classe.bases):
            continue
        achou = True
        metodos += [f.name for f in classe.body if isinstance(f, (ast.FunctionDef, ast.AsyncFunctionDef))]
    if not achou:
        return None, "nenhuma classe TestCase"
    return [m for m in metodos if not excluida_py(m)], None
