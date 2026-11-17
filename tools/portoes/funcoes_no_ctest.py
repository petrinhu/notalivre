#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão das funções declaradas e registradas no CTest (NL-2.20 S2, D13 a D17).

Confronta, programa a programa, as funções declaradas no fonte (Qt Test pela seção private slots; Python pelo ast) com as
registradas por função no CTest. Um processo ctest por rodada e nenhum por programa (L-11, item 6).

Esta entrada só encadeia os módulos de tools/portoes/funcoes_ctest/.
Uso: python3 -I tools/portoes/funcoes_no_ctest.py --raiz R --build B --excecoes L [--json J]
rc: 0 limpo; 1 divergência, varredura vazia, não analisado, duplicada ou comando não reconhecido; 2 entrada ilegível,
com as contagens todas em 0.
"""

import argparse
import sys
from pathlib import Path

# O python3 -I não põe a pasta do script no sys.path, então os módulos de funcoes_ctest são achados por este caminho.
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent / "funcoes_ctest"))

from comparacao import comparar, mensagens_de_piso  # noqa: E402
from excecoes import ler_excecoes  # noqa: E402
from fontes import declaradas_por_programa  # noqa: E402
from parada import Parada  # noqa: E402
from registro_ctest import agrupar_registro, ler_registro  # noqa: E402

CHAVES_TOTAIS = (
    "ctest", "por_funcao", "inteiros", "outros", "programas_qt", "programas_py", "declaradas",
    "registradas", "excecoes", "faltam", "sobram", "encontrados", "analisados", "falharam",
)
DIVERGENCIAS = (
    "falta:", "sobra:", "excecao morta:", "duplicada:", "comando nao reconhecido:", "nao analisado:", "varredura ",
)


def ler_argumentos(argv):
    """Lê as opções do portão."""
    parser = argparse.ArgumentParser(description="Funções declaradas e registradas no CTest.")
    parser.add_argument("--raiz", required=True)
    parser.add_argument("--build", required=True)
    parser.add_argument("--excecoes", required=True)
    parser.add_argument("--json")
    return parser.parse_args(argv)


def contagens(testes, excecoes, registro, decl, falharam, faltam, sobram):
    """Contagens do resumo, montadas a partir do que cada etapa mediu."""
    return {
        "ctest": len(testes),
        "por_funcao": sum(len(v) for v in registro["por_programa"].values()),
        "inteiros": registro["inteiros"],
        "outros": registro["outros"],
        "programas_qt": sum(1 for t in registro["tipos"].values() if t == "qt"),
        "programas_py": sum(1 for t in registro["tipos"].values() if t == "py"),
        "declaradas": sum(len(v) for v in decl.values()),
        "registradas": sum(len(registro["por_programa"][p]) for p in decl),
        "excecoes": len(excecoes),
        "faltam": faltam,
        "sobram": sobram,
        "encontrados": len(registro["por_programa"]),
        "analisados": len(decl),
        "falharam": falharam,
    }


def varrer(args):
    """Encadeia registro, exceções, análise dos fontes e comparação; devolve as contagens e as mensagens, em ordem."""
    testes = ler_registro(args.json, args.build)
    excecoes = ler_excecoes(args.excecoes)
    registro = agrupar_registro(testes, args.raiz, args.build)
    decl, nao_analisados = declaradas_por_programa(args.raiz, registro["por_programa"], registro["tipos"])
    linhas, faltam, sobram = comparar(decl, registro["por_programa"], excecoes)
    cont = contagens(testes, excecoes, registro, decl, len(nao_analisados), faltam, sobram)
    msgs = registro["mensagens"] + nao_analisados + linhas + mensagens_de_piso(cont)
    return cont, msgs


def codigo_de_saida(msgs):
    """rc 1 quando alguma mensagem é divergência; senão 0."""
    return 1 if any(m.startswith(DIVERGENCIAS) for m in msgs) else 0


def linha_resumo(cont):
    """Linha de resumo com todas as contagens; é sempre a primeira saída."""
    return "funcoes_no_ctest: " + " ".join(f"{chave}={cont[chave]}" for chave in CHAVES_TOTAIS)


def main(argv=None):
    """Ponto de entrada: imprime o resumo na primeira linha e depois as mensagens; devolve o rc."""
    args = ler_argumentos(sys.argv[1:] if argv is None else argv)
    try:
        cont, msgs = varrer(args)
        rc = codigo_de_saida(msgs)
    except Parada as parada:
        cont = {chave: 0 for chave in CHAVES_TOTAIS}
        msgs = [parada.texto]
        rc = 2
    print(linha_resumo(cont))
    for mensagem in msgs:
        print(mensagem)
    return rc


if __name__ == "__main__":
    sys.exit(main())
