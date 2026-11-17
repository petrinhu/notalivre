# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Funções declaradas na seção private slots de um fonte Qt Test, lidas linha a linha."""

import re

EXCLUIDAS_QT = {"initTestCase", "cleanupTestCase", "init", "cleanup"}
SECAO_QT_INICIO = re.compile(r"^private\s+slots\s*:")
SECAO_QT_FIM = re.compile(r"^(public|protected|private)(\s+slots)?\s*:")
FUNCAO_VOID = re.compile(r"^void ([A-Za-z_]\w*)\(\)([ {;].*)?$")


def excluida_qt(nome):
    """Verdadeiro para as funções Qt que não são caso de teste (ciclo de vida e sufixo _data)."""
    return nome in EXCLUIDAS_QT or nome.endswith("_data")


def fim_de_secao(linha):
    """Verdadeiro quando a linha, sem recuo, fecha a seção private slots: fim da classe ou outro rótulo de acesso."""
    return linha.startswith("};") or bool(SECAO_QT_FIM.match(linha))


def slots_qt(linhas):
    """Funções declaradas na seção private slots de um fonte Qt, ou o motivo de não analisar."""
    funcoes, dentro, achou = [], False, False
    for n, linha in enumerate(linhas, 1):
        limpa = linha.strip()
        if not dentro:
            if SECAO_QT_INICIO.match(limpa):
                dentro = achou = True
            continue
        if not limpa:
            continue
        recuo = len(linha) - len(linha.lstrip(" "))
        conteudo = linha.lstrip(" ")
        if recuo > 4:
            continue
        if fim_de_secao(linha):
            dentro = bool(SECAO_QT_INICIO.match(linha))
            continue
        if recuo == 4:
            casou = FUNCAO_VOID.match(conteudo)
            if casou:
                funcoes.append(casou.group(1))
                continue
            if conteudo[:1] in ("{", "}") or conteudo.startswith("//"):
                continue
        return None, f"linha nao reconhecida na linha {n}"
    if not achou:
        return None, "sem secao private slots"
    return [f for f in funcoes if not excluida_qt(f)], None
