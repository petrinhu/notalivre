# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão da variável de SHA-1 (NL-2.6, DA-2.10; controle compensatório do ADR 0007, L-77).

A variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode aparecer no arnês de teste do fedora44 (tools/servidor_falso/oraculo.py),
e nos testes que o exercitam. Qualquer outro arquivo que a cite reprova, porque ela desliga a política de cripto do sistema.

Escopo (L-36, trava nasce global): a árvore inteira da raiz, sem lista de pastas (rodada 5, achado 1).
Exceções, todas escritas aqui e nenhuma outra:
  1. .git: histórico do git, não é fonte.
  2. *.md em docs/ e na raiz (só aí): documentação escrita para leitura. O ADR 0007 e o plano citam a variável por nome,
     e é isso que a exceção protege. Documentação conta à parte, em "documentais=N". docs/ também é a fonte de
     tools/tema/gerar_theme.py, que gera apps/gui/qml/Theme.qml e tests/qml/dados/tokens.json: esses gerados são
     varridos como código, porque não são .md.
  4. worktrees/ NA RAIZ, só o que o git NÃO rastreia: cópias de trabalho do próprio checkout, ignoradas pelo git e pelos
     portões que varrem por caminho (L-55). Arquivo rastreado em worktrees/ continua sendo varrido. Sem repositório git
     (cópia sem .git), nada é podado: a varredura falha fechada. worktrees/ em src/, tools/ etc. também é varrida.
  3. bytecode de Python (pyc): um pyc é derivado de um .py, só vale se o .py de origem for permitido, estiver na árvore e
     também contiver a variável (rodada 5, achados 2 e 3). Nome fora do formato <stem>.cpython-<NN>.pyc, dentro de
     __pycache__ da pasta da origem, não é derivado: é arquivo comum.
  5. Pasta de build do CMake, SÓ por argumento: RAIZ BUILD. O CTest passa o CMAKE_BINARY_DIR, e a pasta é excluída só se
     (a) estiver dentro da RAIZ, (b) tiver CMakeCache.txt, e (c) esse CMakeCache.txt declarar a própria pasta em
     CMAKE_CACHEFILE_DIR. Sem argumento, nada é excluído: o job higiene (sem argumento) reprova qualquer arquivo sob
     _build, versionado ou não. Argumento que não cumpre (a), (b) ou (c) é recusado com rc 2, nunca varrido em silêncio.
     Motivo (run 41179570887): o build do Flatpak roda dentro da árvore (cmake -B _build na raiz da sandbox), e o objeto
     e o binário do teste permitido (tst_assinador_xml.cpp) carregam o nome da variável. Só o diretório de build sai;
     árvore de código, src/_build e _build sem cache própria continuam varridos.

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M violacoes=V permitidos=P derivados=D documentais=O".
Sai com 1 se houver violação, se algum arquivo listado não puder ser lido, se a varredura achar zero arquivos
(varredura quebrada, não código limpo) ou se a contagem de analisados diferir da de encontrados. Sai com 2 se o
argumento BUILD for recusado.

Uso: python3 -I portao_sha1_env.py RAIZ_DO_REPOSITORIO [BUILD_DO_CMAKE]
"""

import os
import re
import subprocess
import sys

VARIAVEL = "OPENSSL_ENABLE_SHA1_SIGNATURES".encode("ascii")
PERMITIDOS = frozenset({
    "tools/servidor_falso/oraculo.py",
    "tools/portoes/portao_sha1_env.py",
    "tests/tools/servidor_falso/test_servidor_falso.py",
    "tests/tools/portoes/test_portao_sha1_env.py",
    "tests/tools/servidor_falso/CMakeLists.txt",
    # DA-2.37: o teste do assinador XMLDSIG define o nome da variável para provar que o comportamento do app não
    # muda com ela (o assinador nunca a lê nem a define). Permitido pelo caminho exato, nunca por padrão de pasta.
    "tests/unit/foundation/xmldsig/tst_assinador_xml.cpp",
})
PODAR = frozenset({".git"})  # só o histórico do git (exceção 1)
PODAR_NA_RAIZ = frozenset({"worktrees"})  # cópias de trabalho, só na raiz e só não rastreadas (exceção 4, L-55)
RE_PYC = re.compile(r"^(?P<stem>[A-Za-z_][A-Za-z0-9_]*)\.cpython-\d+\.pyc$")  # stem sem ponto (rodada 5, achado 2)
CHAVE_DIR_CACHE = "CMAKE_CACHEFILE_DIR:INTERNAL="  # pasta do build declarada pelo próprio CMake (exceção 5)


def _podar(nome):
    return nome in PODAR


def rastreados(raiz):
    """Caminhos que o git rastreia (índice), ou None se a raiz não tem repositório git (falha fechada)."""
    if not os.path.exists(os.path.join(raiz, ".git")):
        return None
    r = subprocess.run(["git", "-C", raiz, "ls-files", "-z"], capture_output=True)
    if r.returncode != 0:
        return None
    return {p for p in r.stdout.decode("utf-8", errors="surrogateescape").split("\0") if p}


def _cache_declara(cache):
    """Valor de CMAKE_CACHEFILE_DIR no CMakeCache.txt, ou None se a linha não existe."""
    with open(cache, encoding="utf-8", errors="replace") as f:
        for linha in f:
            if linha.startswith(CHAVE_DIR_CACHE):
                return linha[len(CHAVE_DIR_CACHE):].strip()
    return None


def excluir_build(raiz, build):
    """Exceção 5: devolve o caminho real da pasta de build a excluir, ou levanta ValueError (argumento recusado).

    Recusa se a pasta estiver fora da raiz (ou for a própria raiz), se não tiver CMakeCache.txt, ou se o cache dela
    não declarar a própria pasta em CMAKE_CACHEFILE_DIR.
    """
    raiz_real = os.path.realpath(raiz)
    alvo = os.path.realpath(build)
    if alvo == raiz_real or os.path.commonpath([alvo, raiz_real]) != raiz_real:
        raise ValueError(f"pasta de build fora da raiz da varredura: {build}")
    cache = os.path.join(alvo, "CMakeCache.txt")
    if not os.path.isfile(cache):
        raise ValueError(f"pasta de build sem CMakeCache.txt próprio: {build}")
    declarado = _cache_declara(cache)
    if declarado is None or os.path.realpath(declarado) != alvo:
        raise ValueError(f"CMakeCache.txt de {build} não declara esta pasta em CMAKE_CACHEFILE_DIR")
    return alvo


def listar(raiz, excluido=None):
    """Caminhos relativos (barra sempre) de todos os arquivos da raiz, em ordem.

    Só .git é podado. Com excluido (caminho real de uma pasta de build validada por excluir_build), essa pasta sai
    da varredura (exceção 5). Em worktrees/ da raiz, só os arquivos não rastreados saem da varredura (exceção 4).
    """
    tracked = rastreados(raiz)
    caminhos = []
    for atual, dirs, arquivos in os.walk(raiz):
        dirs[:] = [d for d in dirs if not _podar(d)
                   and (excluido is None or os.path.realpath(os.path.join(atual, d)) != excluido)]
        for arquivo in arquivos:
            caminho = os.path.relpath(os.path.join(atual, arquivo), raiz).replace(os.sep, "/")
            if caminho.startswith("worktrees/") and tracked is not None and caminho not in tracked:
                continue  # exceção 4: cópia de trabalho não rastreada
            caminhos.append(caminho)
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
    """Exceção 2: .md em docs/ ou na raiz. Markdown em src/ ou em outra pasta é código de varredura."""
    return rel.endswith(".md") and (rel.startswith("docs/") or "/" not in rel)


def analisar(raiz, excluido=None):
    """Devolve (encontrados, analisados, violacoes, permitidos, derivados, documentais, lista_violacoes)."""
    arquivos = listar(raiz, excluido)
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
    if len(argv) not in (2, 3):
        print("uso: portao_sha1_env.py RAIZ [BUILD_DO_CMAKE]", file=sys.stderr)
        return 2
    excluido = None
    if len(argv) == 3:
        try:
            excluido = excluir_build(argv[1], argv[2])
        except ValueError as erro:
            print(f"PARADA: argumento BUILD recusado: {erro}", file=sys.stderr)
            return 2
    enc, ana, viol, perm, der, doc, lista = analisar(argv[1], excluido)
    if excluido is not None:
        print(f"build excluído (exceção 5, CMakeCache.txt declara a pasta): {excluido}")
    print(f"encontrados={enc} analisados={ana} violacoes={viol} permitidos={perm} derivados={der} documentais={doc}")
    for rel in lista:
        print(f"VIOLACAO {rel}: a variável OPENSSL_ENABLE_SHA1_SIGNATURES só pode existir no arnês do fedora44")
    if enc == 0 or ana != enc or viol:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
