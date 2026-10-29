# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão da variável de SHA-1 (NL-2.6, DA-2.10; controle compensatório do ADR 0007, L-77).

A variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode aparecer no arnês de teste do fedora44 (tools/servidor_falso/oraculo.py),
e nos testes que o exercitam. Qualquer outro arquivo que a cite reprova, porque ela desliga a política de cripto do sistema.

Escopo (L-36, trava nasce global): a árvore inteira da raiz, sem lista de pastas (rodada 5, achado 1).
Exceções, todas escritas aqui e nenhuma outra:
  1. .git: histórico do git, não é fonte.
  2. *.md: documentação em Markdown, nunca compilada nem executada pelo build ou pelo CI. O ADR 0007 e o plano citam a
     variável por nome, e é isso que a exceção protege. Documentação conta à parte, em "documentais=N".
  4. worktrees/ NA RAIZ (só na raiz, não em outro nível): cópias de trabalho do próprio checkout, ignoradas pelo git e
     pelos portões que varrem por caminho (L-55). Sem essa exceção, o portão rodado na raiz do checkout principal
     reprovaria por falso positivo. worktrees/ em src/, tools/ etc. continua sendo varrida.
  3. bytecode de Python (pyc): um pyc é derivado de um .py, só vale se o .py de origem for permitido, estiver na árvore e
     também contiver a variável (rodada 5, achados 2 e 3). Nome fora do formato <stem>.cpython-<NN>.pyc, dentro de
     __pycache__ da pasta da origem, não é derivado: é arquivo comum.

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M violacoes=V permitidos=P derivados=D documentais=O".
Sai com 1 se houver violação, se algum arquivo listado não puder ser lido, se a varredura achar zero arquivos
(varredura quebrada, não código limpo) ou se a contagem de analisados diferir da de encontrados.

Uso: python3 -I portao_sha1_env.py RAIZ_DO_REPOSITORIO
"""

import os
import re
import sys

VARIAVEL = "OPENSSL_ENABLE_SHA1_SIGNATURES".encode("ascii")
PERMITIDOS = frozenset({
    "tools/servidor_falso/oraculo.py",
    "tools/portoes/portao_sha1_env.py",
    "tests/tools/servidor_falso/test_servidor_falso.py",
    "tests/tools/portoes/test_portao_sha1_env.py",
    "tests/tools/servidor_falso/CMakeLists.txt",
})
PODAR = frozenset({".git"})  # só o histórico do git (exceção 1)
PODAR_NA_RAIZ = frozenset({"worktrees"})  # cópias de trabalho, só na raiz (exceção 4, L-55)
RE_PYC = re.compile(r"^(?P<stem>[A-Za-z_][A-Za-z0-9_]*)\.cpython-\d+\.pyc$")  # stem sem ponto (rodada 5, achado 2)


def _podar(nome):
    return nome in PODAR


def listar(raiz):
    """Caminhos relativos (barra sempre) de todos os arquivos da raiz, em ordem. Só .git é podado."""
    caminhos = []
    topo = os.path.normpath(raiz)
    for atual, dirs, arquivos in os.walk(raiz):
        podar_aqui = (lambda d: _podar(d) or d in PODAR_NA_RAIZ) if os.path.normpath(atual) == topo else _podar
        dirs[:] = [d for d in dirs if not podar_aqui(d)]
        for arquivo in arquivos:
            caminho = os.path.relpath(os.path.join(atual, arquivo), raiz)
            caminhos.append(caminho.replace(os.sep, "/"))
    return sorted(caminhos)


def origem_do_pyc(rel):
    """Caminho do .py de origem de um bytecode <stem>.cpython-<NN>.pyc dentro de __pycache__, ou None."""
    partes = rel.split("/")
    if len(partes) < 2 or partes[-2] != "__pycache__":
        return None
    m = RE_PYC.match(partes[-1])
    if m is None:
        return None
    dono = "/".join(partes[:-2])
    nome = m.group("stem") + ".py"
    return f"{dono}/{nome}" if dono else nome


def _documental(rel):
    return rel.endswith(".md")  # exceção 2


def analisar(raiz):
    """Devolve (encontrados, analisados, violacoes, permitidos, derivados, documentais, lista_violacoes)."""
    arquivos = listar(raiz)
    lidos = {}
    for rel in arquivos:
        try:
            with open(os.path.join(raiz, rel), "rb") as f:
                lidos[rel] = f.read()
        except OSError:
            continue
    com_variavel = {rel for rel, dados in lidos.items() if VARIAVEL in dados}
    permitidos = derivados = documentais = 0
    violacoes = []
    for rel in sorted(com_variavel):
        origem = origem_do_pyc(rel)
        if origem is not None and origem in PERMITIDOS and origem in com_variavel:
            derivados += 1  # exceção 3: bytecode de fonte permitida que também contém a variável
        elif rel in PERMITIDOS:
            permitidos += 1
        elif _documental(rel):
            documentais += 1  # exceção 2
        else:
            violacoes.append(rel)
    return len(arquivos), len(lidos), len(violacoes), permitidos, derivados, documentais, violacoes


def main(argv):
    if len(argv) != 2:
        print("uso: portao_sha1_env.py RAIZ", file=sys.stderr)
        return 2
    enc, ana, viol, perm, der, doc, lista = analisar(argv[1])
    print(f"encontrados={enc} analisados={ana} violacoes={viol} permitidos={perm} derivados={der} documentais={doc}")
    for rel in lista:
        print(f"VIOLACAO {rel}: a variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode existir no arnês do fedora44")
    if enc == 0 or ana != enc or viol:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
