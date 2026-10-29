# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão da variável de SHA-1 (NL-2.6, DA-2.10; controle compensatório do ADR 0007, L-77).

A variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode aparecer no arnês de teste do fedora44 (tools/servidor_falso/oraculo.py).
Qualquer outro arquivo de código que a cite reprova, porque ela desliga a política de cripto do sistema.

Varredura: src/, apps/, flatpak/, .github/, tools/, tests/ e CMakeLists.txt, CMakePresets.json na raiz. Só .git é podado.
Documentos (docs/) não entram: o ADR cita a variável por nome.

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M violacoes=V permitidos=P".
Sai com 1 se houver violação, se algum arquivo listado não puder ser lido, se a varredura achar zero arquivos
(varredura quebrada, não código limpo) ou se a contagem de analisados diferir da de encontrados.

Uso: python3 -I portao_sha1_env.py RAIZ_DO_REPOSITORIO
"""

import os
import sys

VARIAVEL = "OPENSSL_ENABLE_SHA1_SIGNATURES".encode("ascii")
RAIZES = ("src", "apps", "flatpak", ".github", "tools", "tests")
ARQUIVOS_SOLTOS = ("CMakeLists.txt", "CMakePresets.json")
PERMITIDOS = frozenset({
    "tools/servidor_falso/oraculo.py",
    "tools/portoes/portao_sha1_env.py",
    "tests/tools/servidor_falso/test_servidor_falso.py",
    "tests/tools/portoes/test_portao_sha1_env.py",
    "tests/tools/servidor_falso/CMakeLists.txt",
})
PODAR = frozenset({".git"})  # só o histórico do git. Nenhum outro nome de diretório é isenção (rodada 2, item 1).


def _podar(nome):
    return nome in PODAR


def listar(raiz):
    """Caminhos relativos (barra inclusive no Windows) dos arquivos da varredura, em ordem."""
    caminhos = []
    for nome in ARQUIVOS_SOLTOS:
        if os.path.isfile(os.path.join(raiz, nome)):
            caminhos.append(nome)
    for pasta in RAIZES:
        base = os.path.join(raiz, pasta)
        if not os.path.isdir(base):
            continue
        for atual, dirs, arquivos in os.walk(base):
            dirs[:] = [d for d in dirs if not _podar(d)]
            for arquivo in arquivos:
                caminho = os.path.relpath(os.path.join(atual, arquivo), raiz)
                caminhos.append(caminho.replace(os.sep, "/"))
    return sorted(caminhos)


def origem_do_pyc(rel):
    """Caminho do .py de origem de um bytecode de __pycache__ (pacote.cpython-XY.pyc), ou None se não for pyc."""
    partes = rel.split("/")
    if not rel.endswith(".pyc") or "__pycache__" not in partes:
        return None
    idx = len(partes) - 1 - partes[::-1].index("__pycache__")
    dono = "/".join(partes[:idx])
    nome = partes[-1].split(".")[0] + ".py"
    return f"{dono}/{nome}" if dono else nome


def analisar(raiz):
    """Devolve (encontrados, analisados, violacoes, permitidos, derivados, lista_violacoes).

    Um .pyc herda a permissão do .py de origem (que também é varrido), e só se a origem estiver em PERMITIDOS. Pyc órfão
    ou de fonte fora da lista continua sendo analisado como arquivo comum (rodada 4: o CI reprovou o .pyc do oraculo).
    """
    encontrados = analisados = permitidos = derivados = 0
    violacoes = []
    for rel in listar(raiz):
        encontrados += 1
        try:
            with open(os.path.join(raiz, rel), "rb") as f:
                dados = f.read()
        except OSError:
            continue
        analisados += 1
        if VARIAVEL not in dados:
            continue
        if origem_do_pyc(rel) in PERMITIDOS:
            derivados += 1
        elif rel in PERMITIDOS:
            permitidos += 1
        else:
            violacoes.append(rel)
    return encontrados, analisados, len(violacoes), permitidos, derivados, violacoes


def main(argv):
    if len(argv) != 2:
        print("uso: portao_sha1_env.py RAIZ", file=sys.stderr)
        return 2
    enc, ana, viol, perm, der, lista = analisar(argv[1])
    print(f"encontrados={enc} analisados={ana} violacoes={viol} permitidos={perm} derivados={der}")
    for rel in lista:
        print(f"VIOLACAO {rel}: a variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode existir no arnês do fedora44")
    if enc == 0 or ana != enc or viol:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
