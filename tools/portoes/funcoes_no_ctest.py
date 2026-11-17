#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão das funções declaradas e registradas no CTest (NL-2.20 S2, D13 a D17).

Confronta, programa a programa, as funções declaradas no fonte (Qt Test pela seção private slots; Python pelo ast) com as
registradas por função no CTest. Um processo ctest por rodada e nenhum por programa (L-11, item 6).

Uso: python3 -I tools/portoes/funcoes_no_ctest.py --raiz R --build B --excecoes L [--json J]
rc: 0 limpo; 1 divergência, varredura vazia, não analisado, duplicada ou comando não reconhecido; 2 entrada ilegível.
"""

import argparse
import ast
import json
import os
import re
import subprocess
import sys

EXCLUIDAS_QT = {"initTestCase", "cleanupTestCase", "init", "cleanup"}
EXCLUIDAS_PY = {"setUp", "tearDown", "setUpClass", "tearDownClass"}
IDENTIFICADOR = re.compile(r"^[A-Za-z_]\w*$")
SECAO_QT_INICIO = re.compile(r"^private\s+slots\s*:")
SECAO_QT_FIM = re.compile(r"^(public|protected|private)(\s+slots)?\s*:")
FUNCAO_VOID = re.compile(r"^void ([A-Za-z_]\w*)\(\)([ {;].*)?$")
CHAVES_TOTAIS = (
    "ctest", "por_funcao", "inteiros", "outros", "programas_qt", "programas_py", "declaradas",
    "registradas", "excecoes", "faltam", "sobram", "encontrados", "analisados", "falharam",
)
DIVERGENCIAS = (
    "falta:", "sobra:", "excecao morta:", "duplicada:", "comando nao reconhecido:", "nao analisado:", "varredura ",
)


class Parada(Exception):
    """Interrompe a varredura com rc 2 e uma mensagem."""

    def __init__(self, texto):
        super().__init__(texto)
        self.texto = texto


def ler_argumentos(argv):
    """Lê as opções do portão."""
    parser = argparse.ArgumentParser(description="Funções declaradas e registradas no CTest.")
    parser.add_argument("--raiz", required=True)
    parser.add_argument("--build", required=True)
    parser.add_argument("--excecoes", required=True)
    parser.add_argument("--json")
    return parser.parse_args(argv)


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


def ler_registro(json_path, build):
    """Devolve a lista de testes do CTest, do JSON dado ou de um ctest rodado agora; entrada ilegível é Parada."""
    if json_path:
        try:
            with open(json_path, encoding="utf-8") as arq:
                texto = arq.read()
        except (OSError, ValueError):
            raise Parada(f"JSON ilegivel: {json_path}")
    else:
        texto = rodar_ctest(build)
    try:
        dados = json.loads(texto)
    except ValueError:
        raise Parada("JSON ilegivel")
    testes = dados.get("tests") if isinstance(dados, dict) else None
    if not isinstance(testes, list):
        raise Parada("JSON sem lista tests")
    return testes


def classificar_teste(teste):
    """Diz se o teste é registro Qt inteiro, Qt por função, Python por função, outro ou não reconhecido."""
    nome = str(teste.get("name", "?"))
    cmd = teste.get("command")
    if not isinstance(cmd, list) or not cmd or not all(isinstance(t, str) for t in cmd):
        raise Parada(f"teste sem command legivel: {nome}")
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


def fora_da_raiz(programa):
    """Verdadeiro quando o nome do programa sai da raiz ou da pasta de build."""
    return programa.split(os.sep)[0] == ".."


def excluida_qt(nome):
    """Verdadeiro para as funções Qt que não são caso de teste (ciclo de vida e sufixo _data)."""
    return nome in EXCLUIDAS_QT or nome.endswith("_data")


def excluida_py(nome):
    """Verdadeiro para os métodos Python que não são caso de teste (ciclo de vida e nome com _)."""
    return nome.startswith("_") or nome in EXCLUIDAS_PY


def slots_qt(linhas):
    """Funções declaradas na seção private slots de um fonte Qt, ou o motivo de não analisar."""
    funcoes, dentro, achou = [], False, False
    for n, linha in enumerate(linhas, 1):
        limpa = linha.strip()
        if not dentro:
            if SECAO_QT_INICIO.match(limpa):
                dentro = achou = True
            continue
        if limpa.startswith("};") or SECAO_QT_FIM.match(limpa):
            dentro = False
            continue
        if not limpa:
            continue
        recuo = len(linha) - len(linha.lstrip(" "))
        conteudo = linha.lstrip(" ")
        if recuo > 4:
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


def acumular(reg, tipos, programa, tipo, funcao, msgs):
    """Guarda um registro por função; a mesma função duas vezes no mesmo programa vira duplicada."""
    tipos[programa] = tipo
    lista = reg.setdefault(programa, [])
    if funcao in lista:
        msgs.append(f"duplicada: {programa}::{funcao}")
    lista.append(funcao)


def conferir_piso(cont, msgs):
    """Varredura vazia reprova; analisados diferente de encontrados também."""
    if cont["ctest"] == 0 or cont["por_funcao"] == 0:
        msgs.append("varredura vazia")
    elif cont["analisados"] != cont["encontrados"]:
        msgs.append(f"varredura incompleta: analisados={cont['analisados']} de encontrados={cont['encontrados']}")


def executar(args, cont, msgs):
    """Roda a varredura inteira e devolve o rc; preenche as contagens e as mensagens."""
    testes = ler_registro(args.json, args.build)
    excecoes = ler_excecoes(args.excecoes)
    cont["ctest"] = len(testes)
    cont["excecoes"] = len(excecoes)
    tipos, reg = {}, {}
    for teste in testes:
        tipo, a, funcao = classificar_teste(teste)
        if tipo == "inteiro":
            cont["inteiros"] += 1
        elif tipo == "outro":
            cont["outros"] += 1
        elif tipo == "nao_reconhecido":
            msgs.append(f"comando nao reconhecido: {a}")
        else:
            programa = programa_qt(args.build, a) if tipo == "qt" else programa_py(args.raiz, a)
            acumular(reg, tipos, programa, tipo, funcao, msgs)
    cont["por_funcao"] = sum(len(v) for v in reg.values())
    cont["programas_qt"] = sum(1 for t in tipos.values() if t == "qt")
    cont["programas_py"] = sum(1 for t in tipos.values() if t == "py")
    cont["encontrados"] = len(reg)
    decl = {}
    for programa in sorted(reg):
        declaradas, motivo = analisar_programa(args.raiz, programa, tipos[programa])
        if motivo is not None:
            msgs.append(f"nao analisado: {programa}: {motivo}")
        else:
            decl[programa] = declaradas
    cont["analisados"] = len(decl)
    cont["falharam"] = sum(1 for m in msgs if m.startswith("nao analisado:"))
    cont["declaradas"] = sum(len(v) for v in decl.values())
    cont["registradas"] = sum(len(reg[p]) for p in decl)
    linhas, cont["faltam"], cont["sobram"] = comparar(decl, reg, excecoes)
    msgs.extend(linhas)
    conferir_piso(cont, msgs)
    return 1 if any(m.startswith(DIVERGENCIAS) for m in msgs) else 0


def linha_resumo(cont):
    """Linha de resumo com todas as contagens; é sempre a primeira saída."""
    return "funcoes_no_ctest: " + " ".join(f"{chave}={cont[chave]}" for chave in CHAVES_TOTAIS)


def main(argv=None):
    """Ponto de entrada: imprime o resumo na primeira linha e depois as mensagens; devolve o rc."""
    args = ler_argumentos(sys.argv[1:] if argv is None else argv)
    cont = {chave: 0 for chave in CHAVES_TOTAIS}
    msgs = []
    try:
        rc = executar(args, cont, msgs)
    except Parada as parada:
        rc = 2
        msgs.append(parada.texto)
    print(linha_resumo(cont))
    for mensagem in msgs:
        print(mensagem)
    return rc


if __name__ == "__main__":
    sys.exit(main())
