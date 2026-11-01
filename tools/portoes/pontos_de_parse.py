#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão de pontos de parse da libxml2 (NL-2.21, DA-2.67).

Varre src/ e conta as chamadas de entrada de parse da libxml2 (lista em RE_CHAMADA). Compara, por igualdade exata, o
conjunto de (arquivo, função chamada) com a lista versionada tools/portoes/pontos_de_parse.txt. Qualquer chamada nova
fora da lista reprova; qualquer linha da lista sem chamada reprova; varredura vazia reprova (zero é varredura quebrada).

Comentários e literais de texto saem antes da busca: um nome de função citado num comentário não conta.

Uso: python3 -I pontos_de_parse.py RAIZ [LISTA]
  RAIZ   pasta cuja src/ é varrida (a árvore do commit a verificar)
  LISTA  padrão: pontos_de_parse.txt ao lado DESTE script, nunca dentro da árvore varrida. Assim o portão rodado contra
         uma árvore antiga (estreia vermelha) compara com a lista atual, e não com a lista que a árvore antiga não tem.
Saída: "encontrados=N permitidos=M divergencias=D", depois uma linha "divergencia: ..." por divergência.
Sai com 0 se limpo; 1 se houver divergência ou varredura vazia; 2 se a lista não abre ou não tem o formato.
"""

import os
import re
import sys

# Entradas de parse da libxml2 que o portão conta (lista do NL-2.21, ordem da ordem de serviço). xmlCtxtRead* cobre
# a família inteira. A busca exige "(" logo após o nome, então xmlParseDocument não casa com xmlParseDoc.
FUNCOES_DE_PARSE = (
    "xmlReadMemory", "xmlReadFile", "xmlReadDoc",
    "xmlParseMemory", "xmlParseFile", "xmlParseDoc",
    "xmlCreateMemoryParserCtxt", "xmlCreatePushParserCtxt", "xmlNewParserCtxt",
    "xmlSchemaNewParserCtxt", "xmlSchemaNewMemParserCtxt",
    "xmlRelaxNGNewParserCtxt",
)
RE_CHAMADA = re.compile(r"\b(xmlCtxtRead\w*|" + "|".join(FUNCOES_DE_PARSE) + r")\s*\(")
# Literal de texto ou comentário. O literal é mantido (pode conter "//"); o comentário sai, preservando as quebras de linha.
RE_LITERAL_OU_COMENTARIO = re.compile(r'"(?:\\.|[^"\\\n])*"|/\*.*?\*/|//[^\n]*', re.S)
EXTENSOES = (".cpp", ".cc", ".cxx", ".h", ".hpp")


def sem_comentarios(texto):
    def trocar(m):
        if m.group(0).startswith('"'):
            return m.group(0)
        return "\n" * m.group(0).count("\n")

    return RE_LITERAL_OU_COMENTARIO.sub(trocar, texto)


def encontrar(raiz):
    """Lista de (arquivo relativo à raiz, função) com uma entrada por chamada."""
    achados = []
    base = os.path.join(raiz, "src")
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames.sort()
        for nome in sorted(filenames):
            if not nome.endswith(EXTENSOES):
                continue
            caminho = os.path.join(dirpath, nome)
            relativo = os.path.relpath(caminho, raiz).replace(os.sep, "/")
            with open(caminho, encoding="utf-8", errors="replace") as arquivo:
                texto = arquivo.read()
            for m in RE_CHAMADA.finditer(sem_comentarios(texto)):
                achados.append((relativo, m.group(1)))
    return achados


def ler_lista(caminho):
    """Pares (arquivo, função) da lista. Formato por linha: arquivo | função | motivo. Linha vazia ou com # é ignorada."""
    pares = []
    with open(caminho, encoding="utf-8") as arquivo:
        for numero, linha in enumerate(arquivo, start=1):
            texto = linha.strip()
            if not texto or texto.startswith("#"):
                continue
            partes = [p.strip() for p in texto.split("|", 2)]
            if len(partes) != 3 or not all(partes):
                raise ValueError(f"linha {numero} sem os três campos (arquivo | função | motivo): {texto}")
            pares.append((partes[0], partes[1]))
    return pares


def main(argv):
    if len(argv) not in (2, 3):
        print("uso: pontos_de_parse.py RAIZ [LISTA]")
        return 2
    raiz = argv[1]
    lista = argv[2] if len(argv) == 3 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "pontos_de_parse.txt")
    try:
        permitidos = ler_lista(lista)
    except (OSError, ValueError) as erro:
        print(f"PARADA: lista de pontos de parse ilegível ({lista}): {erro}")
        return 2

    achados = encontrar(raiz)
    if not achados:
        print("PARADA: varredura vazia, nenhum ponto de parse em src/ (varredura quebrada, nunca árvore limpa)")
        return 1

    conjunto_achados = set(achados)
    conjunto_permitidos = set(permitidos)
    divergencias = []
    if len(conjunto_permitidos) != len(permitidos):
        divergencias.append("lista com linha repetida")
    for arquivo, funcao in sorted(conjunto_achados - conjunto_permitidos):
        divergencias.append(f"chamada fora da lista: {arquivo} {funcao}")
    for arquivo, funcao in sorted(conjunto_permitidos - conjunto_achados):
        divergencias.append(f"linha da lista sem chamada: {arquivo} {funcao}")

    print(f"encontrados={len(achados)} permitidos={len(permitidos)} divergencias={len(divergencias)}")
    for texto in divergencias:
        print(f"divergencia: {texto}")
    return 1 if divergencias else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
