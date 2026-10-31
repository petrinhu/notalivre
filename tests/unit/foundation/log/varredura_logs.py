#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Varredura de logs de PII do NotaLivre (NL-2.7, ADR-S4). Uma invocação, uma contagem sempre.

Uso: python3 -I varredura_logs.py --dir DIRETORIO --termos ARQUIVO_TERMOS [--termos OUTRO ...]

  --termos pode repetir: a lista final é a soma dos arquivos (NL-2.9, item 10: a referência fixa mais a da execução).

  DIRETORIO: cada *.log do nível de cima é um arquivo analisado.
  ARQUIVO_TERMOS: linhas `categoria|valor`; linhas vazias e as que começam com `#` são ignoradas.

Saída (sempre): encontrados=N analisados=M falharam=K linhas=L ocorrencias=O
Cada ocorrência sai como `OCORRENCIA categoria=C arquivo=A n=X` (o valor nunca é impresso).
Sai com 1, com a razão em `REPROVA: ...`, se: nenhum .log encontrado (varredura vazia, L-36); analisados diferente
de encontrados; zero linhas varridas; falha de leitura; lista de termos vazia ou malformada; diretório ilegível;
ou qualquer ocorrência. Sai com 0 e imprime `VERDE` só quando tudo confere.
Uma só invocação percorre todos os arquivos, sem processo por arquivo (L-11).
"""
import argparse
import os
import sys


def carregar_termos(caminho):
    termos = []
    with open(caminho, encoding="utf-8") as arquivo:
        for numero, linha in enumerate(arquivo, start=1):
            texto = linha.rstrip("\r\n")
            if not texto or texto.startswith("#"):
                continue
            categoria, sep, valor = texto.partition("|")
            if not sep or not categoria or not valor:
                raise ValueError(f"linha {numero} do termos fora do formato categoria|valor")
            termos.append((categoria, valor.encode("utf-8")))
    return termos


def listar_logs(diretorio):
    with os.scandir(diretorio) as entradas:
        return sorted(e.name for e in entradas if e.is_file() and e.name.endswith(".log"))


def varrer(diretorio, termos):
    arquivos = listar_logs(diretorio)
    encontrados = len(arquivos)
    analisados = 0
    falharam = 0
    linhas = 0
    ocorrencias = []
    for nome in arquivos:
        caminho = os.path.join(diretorio, nome)
        try:
            with open(caminho, "rb") as arquivo:
                conteudo = arquivo.read()
        except OSError as erro:
            falharam += 1
            print(f"ERRO ao ler {nome}: {erro.strerror}")
            continue
        analisados += 1
        linhas += conteudo.count(b"\n")
        suspeito = False
        for categoria, valor in termos:
            n = conteudo.count(valor)
            if n:
                suspeito = True
                ocorrencias.append((categoria, nome, n))
        if suspeito:
            falharam += 1
    return encontrados, analisados, falharam, linhas, ocorrencias


def main(argv):
    parser = argparse.ArgumentParser(description="Varredura de PII nos logs (NL-2.7).")
    parser.add_argument("--dir", required=True)
    parser.add_argument("--termos", required=True, action="append")
    args = parser.parse_args(argv)
    try:
        termos = []
        for caminho_termos in args.termos:
            termos.extend(carregar_termos(caminho_termos))
    except (OSError, ValueError) as erro:
        print("encontrados=0 analisados=0 falharam=0 linhas=0 ocorrencias=0")
        print(f"REPROVA: termos ilegíveis: {erro}")
        return 1
    if not termos:
        print("encontrados=0 analisados=0 falharam=0 linhas=0 ocorrencias=0")
        print("REPROVA: lista de termos vazia (portão sem nada para procurar)")
        return 1
    try:
        encontrados, analisados, falharam, linhas, ocorrencias = varrer(args.dir, termos)
    except OSError as erro:
        print("encontrados=0 analisados=0 falharam=0 linhas=0 ocorrencias=0")
        print(f"REPROVA: diretório ilegível: {erro.strerror}")
        return 1
    print(f"encontrados={encontrados} analisados={analisados} falharam={falharam} "
          f"linhas={linhas} ocorrencias={len(ocorrencias)}")
    for categoria, nome, n in ocorrencias:
        print(f"OCORRENCIA categoria={categoria} arquivo={nome} n={n}")
    motivos = []
    if encontrados == 0:
        motivos.append("varredura vazia: nenhum .log encontrado")
    if linhas == 0 and encontrados > 0:
        motivos.append("nenhuma linha varrida (arquivos vazios não provam nada)")
    if analisados != encontrados:
        motivos.append(f"{encontrados - analisados} arquivo(s) não analisado(s)")
    if falharam:
        motivos.append(f"{falharam} arquivo(s) com falha de leitura ou PII")
    if ocorrencias:
        motivos.append(f"{len(ocorrencias)} ocorrência(s) de PII")
    if motivos:
        print("REPROVA: " + "; ".join(motivos))
        return 1
    print("VERDE")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
