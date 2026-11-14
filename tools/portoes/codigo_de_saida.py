#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão estático: o código de saída decide todo teste (NL-3.A; NL-I.258 e NL-I.260; L-36). Lê a árvore rastreada.

Com PASS_REGULAR_EXPRESSION casada, o CTest ignora o código de saída (Source/CTest/cmCTestRunTest.cxx, a mesma condição
de 3.28.3 a 4.4.4). WILL_FAIL inverte o resultado, e SKIP_RETURN_CODE e SKIP_REGULAR_EXPRESSION trocam falha por pulo.
Nenhum CMakeLists.txt nem arquivo .cmake rastreado usa essas quatro propriedades fora de linha de comentário. O pulo e a
contagem de funções ficam com tools/portoes/pulos_testes.sh, pelo junitxml.

Saída, sempre: "arquivos=N linhas_com_token=T violacoes=V". Zero arquivos reprova. Um único git ls-files (L-11).

Uso: python3 -I codigo_de_saida.py RAIZ        (verificação)
     python3 -I codigo_de_saida.py --autoteste (controle positivo, uma sabotagem por propriedade, varredura vazia e o
                                                CTest real: sem PASS, o código 1 depois do Totals reprova)
Rc: 0 passa; 1 reprova; 2 uso.
"""

import os
import re
import subprocess
import sys
import tempfile

PROIBIDAS = ("PASS_REGULAR_EXPRESSION", "WILL_FAIL", "SKIP_RETURN_CODE", "SKIP_REGULAR_EXPRESSION")
RE_PROIBIDA = re.compile(r"\b(" + "|".join(PROIBIDAS) + r")\b")
TOTALS = "Totals: 1 passed, 0 failed"


def arquivos_cmake(raiz):
    """Lista os CMakeLists.txt e .cmake rastreados. git ls-files que falha levanta ValueError (fail-closed)."""
    saida = subprocess.run(["git", "-C", raiz, "ls-files", "-z", "--", "*CMakeLists.txt", "*.cmake"],
                           capture_output=True, check=False)
    if saida.returncode != 0:
        raise ValueError("git ls-files falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    return [nome for nome in saida.stdout.decode("utf-8", "replace").split("\0") if nome]


def conferir_arquivo(raiz, nome):
    """Devolve (linhas com token, violações) de um arquivo."""
    try:
        with open(os.path.join(raiz, nome), encoding="utf-8") as fonte:
            linhas = fonte.read().splitlines()
    except (OSError, UnicodeDecodeError) as erro:
        return 0, [f"{nome}: não lido ({erro})"]
    tokens = 0
    violacoes = []
    for numero, linha in enumerate(linhas, start=1):
        casou = RE_PROIBIDA.search(linha)
        if not casou:
            continue
        tokens += 1
        if not linha.lstrip().startswith("#"):
            violacoes.append(f"{nome}:{numero}: {casou.group(1)} decide o teste sem o código de saída")
    return tokens, violacoes


def verificar(raiz):
    nomes = []
    tokens = 0
    violacoes = []
    try:
        nomes = arquivos_cmake(raiz)
    except ValueError as erro:
        violacoes.append(str(erro))
    for nome in nomes:
        do_arquivo, violacoes_do_arquivo = conferir_arquivo(raiz, nome)
        tokens += do_arquivo
        violacoes += violacoes_do_arquivo
    for violacao in violacoes:
        print(f"VIOLACAO {violacao}")
    print(f"arquivos={len(nomes)} linhas_com_token={tokens} violacoes={len(violacoes)}")
    if not nomes:
        print("REPROVA: varredura vazia")
        return 1
    if violacoes:
        print("REPROVA: teste decidido sem o código de saída")
        return 1
    print("PASSA")
    return 0


BOM = 'add_test(NAME um COMMAND um)\nset_tests_properties(um PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL!")\n'
COMENTARIO = "# PASS_REGULAR_EXPRESSION faz o CTest ignorar o código de saída; por isso não se usa.\n"
CASOS = (
    ("positivo", {"CMakeLists.txt": BOM, "tests/CMakeLists.txt": COMENTARIO + BOM}, 0, "PASSA"),
    ("pass_no_cmakelists", {"CMakeLists.txt": BOM,
                            "tests/a/CMakeLists.txt": BOM.replace("FAIL_", 'PASS_REGULAR_EXPRESSION "Totals" FAIL_')},
     1, "tests/a/CMakeLists.txt:2: PASS_REGULAR_EXPRESSION"),
    ("pass_em_arquivo_cmake", {"CMakeLists.txt": BOM,
                               "tests/cmake/x.cmake": 'set_property(TEST um APPEND PROPERTY PASS_REGULAR_EXPRESSION "x")\n'},
     1, "tests/cmake/x.cmake:1: PASS_REGULAR_EXPRESSION"),
    ("will_fail", {"CMakeLists.txt": BOM.replace('"FAIL!"', '"FAIL!" WILL_FAIL TRUE')}, 1, "CMakeLists.txt:2: WILL_FAIL"),
    ("skip_return_code", {"CMakeLists.txt": BOM.replace('"FAIL!"', '"FAIL!" SKIP_RETURN_CODE 77')}, 1,
     "CMakeLists.txt:2: SKIP_RETURN_CODE"),
    ("skip_regular_expression", {"CMakeLists.txt": BOM.replace('"FAIL!"', '"FAIL!" SKIP_REGULAR_EXPRESSION "SKIP"')}, 1,
     "CMakeLists.txt:2: SKIP_REGULAR_EXPRESSION"),
    ("token_com_comentario_no_fim", {"CMakeLists.txt": BOM + "set(x 1) # PASS_REGULAR_EXPRESSION\n"}, 1,
     "CMakeLists.txt:3: PASS_REGULAR_EXPRESSION"),
    ("so_comentario", {"CMakeLists.txt": COMENTARIO + BOM}, 0, "linhas_com_token=1 violacoes=0"),
    ("fora_do_escopo", {"CMakeLists.txt": BOM, "tools/x.py": "PASS_REGULAR_EXPRESSION = 1\n"}, 0,
     "arquivos=1 linhas_com_token=0"),
    ("vazia", {"leia.txt": "nada\n"}, 1, "varredura vazia"),
    ("sem_git", None, 1, "git ls-files falhou"),
)
CASOS_CTEST = (
    ("limpo", "exit 0", "", False),
    ("codigo_conta", "exit 1", "", True),
    ("pass_esconde_o_codigo", "exit 1", f' PASS_REGULAR_EXPRESSION "{TOTALS}"', False),
)


def montar(pasta, arquivos):
    os.makedirs(pasta)
    for relativo, conteudo in arquivos.items():
        caminho = os.path.join(pasta, relativo)
        os.makedirs(os.path.dirname(caminho), exist_ok=True)
        with open(caminho, "w", encoding="utf-8") as saida:
            saida.write(conteudo)
    for comando in (["init", "-q"], ["add", "-A"]):
        subprocess.run(["git", "-C", pasta] + comando, capture_output=True, check=True)


def rodar_caso(base, nome, arquivos):
    pasta = os.path.join(base, nome)
    if arquivos is None:
        os.makedirs(pasta)
    else:
        montar(pasta, arquivos)
    caminho = os.path.join(base, nome + ".txt")
    with open(caminho, "w", encoding="utf-8") as saida:
        antigo = sys.stdout
        sys.stdout = saida
        try:
            rc = verificar(pasta)
        finally:
            sys.stdout = antigo
    with open(caminho, encoding="utf-8") as entrada:
        return rc, entrada.read()


def conferir_ctest_real(pasta):
    """O CTest do próprio job decide: sem PASS, código 1 depois do Totals reprova; com PASS, passa (é o furo)."""
    os.makedirs(pasta)
    linhas = []
    for nome, fim, propriedades, _ in CASOS_CTEST:
        linhas.append(f"add_test([=[{nome}]=] \"/bin/sh\" \"-c\" \"echo '{TOTALS}' && {fim}\")")
        if propriedades:
            linhas.append(f"set_tests_properties([=[{nome}]=] PROPERTIES{propriedades})")
    with open(os.path.join(pasta, "CTestTestfile.cmake"), "w", encoding="utf-8") as arquivo:
        arquivo.write("\n".join(linhas) + "\n")
    violacoes = []
    for nome, _, _, reprova in CASOS_CTEST:
        try:
            rodada = subprocess.run(["ctest", "--test-dir", pasta, "--no-tests=error", "-R", f"^{nome}$"],
                                    capture_output=True, check=False, timeout=120)
        except (OSError, subprocess.TimeoutExpired) as erro:
            violacoes.append(f"ctest {nome}: não rodou ({erro})")
            continue
        certo = (rodada.returncode != 0) == reprova
        esperado = "reprovar" if reprova else "passar"
        print(f"--- ctest {nome}: rc={rodada.returncode} (esperado {esperado}) {'OK' if certo else 'FALHOU'}")
        if not certo:
            violacoes.append(f"ctest {nome}: rc={rodada.returncode}. Saída: {rodada.stdout.decode('utf-8', 'replace')[-400:]}")
    return violacoes


def autoteste():
    falhas = 0
    with tempfile.TemporaryDirectory(prefix="notalivre-codigo-de-saida-") as base:
        os.environ["GIT_CEILING_DIRECTORIES"] = base
        for nome, arquivos, rc_esperado, trecho in CASOS:
            rc, texto = rodar_caso(base, nome, arquivos)
            certo = rc == rc_esperado and trecho in texto
            print(f"--- caso {nome}: rc={rc} (esperado {rc_esperado}) {'OK' if certo else 'FALHOU'}")
            if not certo:
                falhas += 1
                print(texto)
        violacoes_ctest = conferir_ctest_real(os.path.join(base, "ctest_real"))
    for violacao in violacoes_ctest:
        print(f"VIOLACAO {violacao}")
    print(f"casos={len(CASOS)} falharam={falhas} ctest_casos={len(CASOS_CTEST)} ctest_violacoes={len(violacoes_ctest)}")
    return 1 if falhas or violacoes_ctest or not CASOS else 0


def main(argv):
    if len(argv) == 2 and argv[1] == "--autoteste":
        return autoteste()
    if len(argv) == 2:
        return verificar(argv[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
