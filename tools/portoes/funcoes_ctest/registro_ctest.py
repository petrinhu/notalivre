# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Registro do CTest: leitura do json-v1, validação, classificação e agrupamento por função."""

import json
import os
import re
import subprocess

from parada import Parada

IDENTIFICADOR = re.compile(r"^[A-Za-z_]\w*$")


def rodar_ctest(build):
    """Roda o ctest uma vez e devolve a saída; rc diferente de zero vira Parada, e sinal vira 128 + sinal."""
    try:
        proc = subprocess.run(
            ["ctest", "--test-dir", build, "--show-only=json-v1"],
            capture_output=True, text=True, check=False,
        )
    except (OSError, ValueError) as erro:
        raise Parada(f"ctest nao executou: {erro}")
    if proc.returncode != 0:
        codigo = 128 - proc.returncode if proc.returncode < 0 else proc.returncode
        raise Parada(f"ctest saiu com rc={codigo}")
    return proc.stdout


def ler_arquivo_json(json_path):
    """Texto do JSON gravado; arquivo ilegível é Parada."""
    try:
        with open(json_path, encoding="utf-8") as arq:
            return arq.read()
    except (OSError, ValueError):
        raise Parada(f"JSON ilegivel: {json_path}")


def validar_teste(posicao, teste):
    """Confere um item da lista tests: objeto com command em lista não vazia de textos sem byte nulo; senão Parada."""
    if not isinstance(teste, dict):
        raise Parada(f"teste nao e objeto na posicao {posicao}")
    nome = str(teste.get("name", "?"))
    cmd = teste.get("command")
    if not isinstance(cmd, list) or not cmd or not all(isinstance(t, str) for t in cmd):
        raise Parada(f"teste sem command legivel: {nome}")
    if any("\x00" in t for t in cmd):
        raise Parada(f"command com byte nulo: {nome}")


def ler_registro(json_path, build):
    """Devolve a lista de testes do CTest, do JSON dado ou de um ctest rodado agora; entrada ilegível é Parada."""
    texto = ler_arquivo_json(json_path) if json_path else rodar_ctest(build)
    try:
        dados = json.loads(texto)
    except (ValueError, RecursionError):
        raise Parada("JSON ilegivel")
    testes = dados.get("tests") if isinstance(dados, dict) else None
    if not isinstance(testes, list):
        raise Parada("JSON sem lista tests")
    for posicao, teste in enumerate(testes, 1):
        validar_teste(posicao, teste)
    return testes


def classificar_teste(teste):
    """Diz se o teste é registro Qt inteiro, Qt por função, Python por função, outro ou não reconhecido."""
    nome = str(teste.get("name", "?"))
    cmd = teste.get("command")
    if len(cmd) >= 5 and cmd[1:4] == ["-o", "-,txt", "-o"] and cmd[4].endswith(",junitxml"):
        sobra = cmd[5:]
        if not sobra:
            return ("inteiro", None, None)
        if len(sobra) == 1 and IDENTIFICADOR.match(sobra[0]):
            return ("qt", cmd[0], sobra[0])
        return ("nao_reconhecido", nome, None)
    for i, token in enumerate(cmd):
        if token.endswith(".py"):
            if len(cmd) == i + 2 and IDENTIFICADOR.match(cmd[i + 1]):
                return ("py", token, cmd[i + 1])
            break
    return ("outro", None, None)


def programa_qt(build, executavel):
    """Nome do fonte Qt: o executável relativo à pasta de build, com .cpp."""
    relativo = os.path.relpath(os.path.realpath(executavel), os.path.realpath(build))
    return relativo + ".cpp"


def programa_py(raiz, script):
    """Nome do script Python relativo à raiz."""
    return os.path.relpath(os.path.realpath(script), os.path.realpath(raiz))


def acumular(grupo, programa, tipo, funcao):
    """Guarda um registro por função no grupo; a mesma função duas vezes no mesmo programa vira duplicada."""
    grupo["tipos"][programa] = tipo
    lista = grupo["por_programa"].setdefault(programa, [])
    if funcao in lista:
        grupo["mensagens"].append(f"duplicada: {programa}::{funcao}")
    lista.append(funcao)


def agrupar_registro(testes, raiz, build):
    """Registros por função agrupados por programa, com o tipo de cada um, inteiros, outros e mensagens do registro."""
    grupo = {"por_programa": {}, "tipos": {}, "inteiros": 0, "outros": 0, "mensagens": []}
    for teste in testes:
        tipo, valor, funcao = classificar_teste(teste)
        if tipo == "inteiro":
            grupo["inteiros"] += 1
        elif tipo == "outro":
            grupo["outros"] += 1
        elif tipo == "nao_reconhecido":
            grupo["mensagens"].append(f"comando nao reconhecido: {valor}")
        else:
            programa = programa_qt(build, valor) if tipo == "qt" else programa_py(raiz, valor)
            acumular(grupo, programa, tipo, funcao)
    return grupo
