#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão SPDX do NotaLivre (NL-0.2, achado A0-06). Uma invocação, uma contagem sempre.

Uso: python3 -I spdx.py RAIZ [ARQUIVO_EXCECOES]
  RAIZ: árvore a varrer. Com .git, a lista vem de `git ls-files` (uma chamada); sem .git (cópia de
        sabotagem ou exportação), a árvore é percorrida com poda de .git, claude, worktrees e build*.
  ARQUIVO_EXCECOES: padrão spdx_excecoes.txt ao lado deste arquivo (formato: caminho|motivo).

Regra: todo arquivo de código (C++, headers, Python, shell, QML, YAML, CMake, Dockerfile) tem, nas
primeiras 10 linhas, SPDX-License-Identifier: GPL-3.0-or-later e SPDX-FileCopyrightText com o titular.
JSON não tem comentário: só passa se estiver declarado no arquivo de exceções, com motivo. Exceção que
aponta para arquivo inexistente (obsoleta) reprova.

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M codigo=C violacoes=V excecoes=E".
Sai com 1 se: a listagem falhar; o arquivo de exceções for inválido; nada for encontrado (varredura
vazia); nenhum arquivo de código existir (piso); algum arquivo encontrado não for analisado; houver
violação ou exceção obsoleta.
"""

import itertools
import os
import subprocess
import sys

LICENCA = "SPDX-License-Identifier: GPL-3.0-or-later"
TITULAR = "SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa"
LINHAS_CABECALHO = 10
EXT_CODIGO = (".cpp", ".h", ".hpp", ".py", ".sh", ".qml", ".yml", ".yaml", ".cmake")
EXT_JSON = ".json"
PODAR = (".git", ".claude", "worktrees", "out")
DIR_PADRAO_EXCECOES = os.path.dirname(os.path.abspath(__file__))
ARQUIVO_EXCECOES_PADRAO = os.path.join(DIR_PADRAO_EXCECOES, "spdx_excecoes.txt")


def e_codigo(rel):
    nome = os.path.basename(rel)
    return (rel.endswith(EXT_CODIGO) or nome == "CMakeLists.txt"
            or nome == "Dockerfile" or nome.endswith(".Dockerfile"))


def listar(raiz):
    """Devolve (caminhos_relativos, erro). Uma única chamada de git quando há .git."""
    if os.path.exists(os.path.join(raiz, ".git")):
        feito = subprocess.run(["git", "-C", raiz, "ls-files", "-z"], capture_output=True, check=False)
        if feito.returncode != 0:
            return [], f"git ls-files falhou (rc={feito.returncode})"
        bruto = feito.stdout.decode("utf-8", "surrogateescape")
        return [p for p in bruto.split("\0") if p], None
    caminhos = []
    for atual, dirs, nomes in os.walk(raiz):
        dirs[:] = [d for d in dirs if d not in PODAR and not d.startswith("build")]
        for nome in nomes:
            completo = os.path.join(atual, nome)
            caminhos.append(os.path.relpath(completo, raiz).replace(os.sep, "/"))
    return caminhos, None


def ler_excecoes(arquivo):
    """Devolve (dict caminho->motivo, erro). Formato: caminho|motivo; linhas com # são comentário."""
    excecoes = {}
    try:
        with open(arquivo, "r", encoding="utf-8") as fonte:
            linhas = fonte.read().splitlines()
    except OSError as erro:
        return {}, f"arquivo de exceções ilegível: {erro.strerror}"
    for numero, linha in enumerate(linhas, 1):
        texto = linha.strip()
        if not texto or texto.startswith("#"):
            continue
        if "|" not in texto:
            return {}, f"{arquivo}:{numero}: falta o motivo (formato caminho|motivo)"
        caminho, motivo = (parte.strip() for parte in texto.split("|", 1))
        if not caminho or not motivo:
            return {}, f"{arquivo}:{numero}: caminho ou motivo vazio"
        excecoes[caminho] = motivo
    return excecoes, None


def ler_inicio(caminho):
    """Lê o arquivo; devolve as primeiras LINHAS_CABECALHO linhas (levanta OSError se ilegível)."""
    with open(caminho, "rb") as fonte:
        return b"".join(itertools.islice(fonte, LINHAS_CABECALHO)).decode("utf-8", "replace")


def tem_cabecalho(inicio):
    return LICENCA in inicio and TITULAR in inicio


def main(argv):
    if len(argv) not in (1, 2):
        print("uso: spdx.py RAIZ [ARQUIVO_EXCECOES]")
        return 1
    raiz = argv[0]
    arquivo_excecoes = argv[1] if len(argv) == 2 else ARQUIVO_EXCECOES_PADRAO
    if not os.path.isdir(raiz):
        print(f"RAIZ-INEXISTENTE {raiz}")
        print("encontrados=0 analisados=0 codigo=0 violacoes=0 excecoes=0")
        print("REPROVA: raiz inexistente")
        return 1

    excecoes, erro_excecoes = ler_excecoes(arquivo_excecoes)
    caminhos, erro_listagem = listar(raiz)
    conjunto = set(caminhos)
    analisados = codigo = violacoes = usadas = 0
    for rel in caminhos:
        try:
            inicio = ler_inicio(os.path.join(raiz, rel))
        except OSError as erro:
            print(f"NAO-ANALISADO {rel}: {erro.strerror}")
            continue
        analisados += 1
        if rel.endswith(EXT_JSON):
            if rel in excecoes:
                usadas += 1
            else:
                violacoes += 1
                print(f"VIOLACAO {rel}: JSON sem exceção declarada (sem comentário para o cabeçalho)")
        elif e_codigo(rel):
            codigo += 1
            if not tem_cabecalho(inicio):
                violacoes += 1
                print(f"VIOLACAO {rel}: sem cabeçalho SPDX (licença e titular, nas 10 primeiras linhas)")
    for caminho in sorted(excecoes if not erro_listagem else {}):
        if caminho not in conjunto:
            violacoes += 1
            print(f"VIOLACAO {caminho}: exceção obsoleta (arquivo não existe na árvore)")
        elif not caminho.endswith(EXT_JSON):
            violacoes += 1
            print(f"VIOLACAO {caminho}: exceção declarada em arquivo que não é JSON")

    print(f"encontrados={len(caminhos)} analisados={analisados} codigo={codigo} "
          f"violacoes={violacoes} excecoes={usadas}")
    if erro_listagem or erro_excecoes:
        print(f"REPROVA: {erro_listagem or erro_excecoes}")
        return 1
    if not caminhos:
        print("REPROVA: varredura vazia (zero arquivos encontrados)")
        return 1
    if codigo == 0:
        print("REPROVA: piso de código (zero arquivos de código encontrados)")
        return 1
    if analisados != len(caminhos):
        print("REPROVA: arquivo encontrado e não analisado")
        return 1
    if violacoes:
        print("REPROVA: arquivo sem cabeçalho SPDX ou exceção inválida")
        return 1
    print("PASSA")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
