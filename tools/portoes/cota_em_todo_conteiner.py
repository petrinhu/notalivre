#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão estático: todo contêiner desta máquina nasce com o teto de CPU e de memória (NL-3.K; NL-I.266, NL-I.275; L-36).

Decisão do líder, 13/11/2026: "Consertar só o teto (Recomendado)": todo contêiner com teto de CPU e de memória, de um ponto
único, para que nenhum build tome a máquina, sem prender núcleos. O ponto único é tools/conteiner/cota_conteiner.sh, que monta
o vetor NL_COTAS_DOCKER que todo docker run desta máquina recebe.

Regra: em todo arquivo .sh rastreado, toda chamada `docker run` fora de aspas e de comentário traz o vetor
"${NL_COTAS_DOCKER[@]}" no mesmo comando. Linhas com barra invertida no fim se juntam. O .yml do CI fica fora: roda no
runner do GitHub, não nesta máquina.
Exceção declarada, exatamente uma: tools/mutacao/sabotar_portoes.sh, o docker run de "$FLATHUB_KDE" (modo docker, só no CI).
LACUNA DECLARADA (NL-3.K, passo 0): `docker build` não recebe a cota por opção; as chamadas de hoje ficam num teto por
arquivo (LACUNAS_BUILD), e qualquer docker build novo, ou além do teto, reprova.

Saída, sempre: "arquivos=N run=R build=B sem_cota=V excecoes=E lacunas=L". Zero arquivos ou zero chamadas reprova.
LIMITE DECLARADO: comando montado em variável, por eval ou dentro de aspas (sh -c "docker run ...") fica fora da varredura.

Uso: python3 -I cota_em_todo_conteiner.py RAIZ        (verificação)
     python3 -I cota_em_todo_conteiner.py --autoteste (controle positivo, uma sabotagem por forma e varredura vazia)
Rc: 0 passa; 1 reprova; 2 uso.
"""

import os
import re
import subprocess
import sys
import tempfile

VETOR_RUN = '"${NL_COTAS_DOCKER[@]}"'
MARCA_RUN = "@@COTA_RUN@@"
LACUNAS_BUILD = {"tools/conteiner/build_local.sh": 1, "tools/espelho/comum.sh": 2, "tools/portoes/rodar_higiene_conteiner.sh": 1}
RE_CHAMADA = re.compile(r"docker[ \t]+(run|build)\b")
RE_FIM = re.compile(r"&&|\|\||;|\|")
EXCECAO_ARQUIVO = "tools/mutacao/sabotar_portoes.sh"
EXCECAO_TEXTO = '"$FLATHUB_KDE"'
MARCA_EXCECAO = "@@FLATHUB@@"
EXCECOES_ESPERADAS = 1


def sem_aspas_e_comentario(linha):
    """Troca o conteúdo entre aspas por espaço e corta o comentário (# depois de espaço, fora de aspas)."""
    saida = []
    aspa = ""
    for i, c in enumerate(linha):
        if aspa:
            if c == aspa:
                aspa = ""
            saida.append(" ")
            continue
        if c in "\"'":
            aspa = c
            saida.append(" ")
            continue
        if c == "#" and (i == 0 or linha[i - 1] in " \t"):
            break
        saida.append(c)
    return "".join(saida)


def comandos(texto):
    """Devolve (número da primeira linha, comando lógico) com as continuações de barra invertida juntadas."""
    juntos = []
    acumulado = ""
    inicio = 0
    for numero, linha in enumerate(texto.splitlines(), start=1):
        if not acumulado:
            inicio = numero
        if linha.rstrip().endswith("\\"):
            acumulado += linha.rstrip()[:-1] + " "
            continue
        juntos.append((inicio, acumulado + linha))
        acumulado = ""
    if acumulado:
        juntos.append((inicio, acumulado))
    return juntos


def conferir_texto(nome, texto):
    """Devolve (run, build, violações, exceções, lacunas) de um arquivo."""
    run = build = excecoes = lacunas = 0
    violacoes = []
    for numero, comando in comandos(texto):
        if comando.lstrip().startswith("#"):
            continue
        marcado = comando.replace(VETOR_RUN, MARCA_RUN)
        marcado = marcado.replace(EXCECAO_TEXTO, MARCA_EXCECAO)
        limpo = sem_aspas_e_comentario(marcado)
        for casou in RE_CHAMADA.finditer(limpo):
            resto = limpo[casou.end():]
            fim = RE_FIM.search(resto)
            trecho = resto[:fim.start()] if fim else resto
            tipo = casou.group(1)
            if tipo == "run":
                run += 1
                if MARCA_RUN in trecho:
                    continue
                if nome == EXCECAO_ARQUIVO and MARCA_EXCECAO in trecho:
                    excecoes += 1
                    continue
                violacoes.append(f"{nome}:{numero}: docker run sem a cota (vetor {VETOR_RUN})")
                continue
            build += 1
            if lacunas < LACUNAS_BUILD.get(nome, 0):
                lacunas += 1
                continue
            violacoes.append(f"{nome}:{numero}: docker build fora da lacuna declarada (LACUNAS_BUILD)")
    return run, build, violacoes, excecoes, lacunas


def arquivos_sh(raiz):
    saida = subprocess.run(["git", "-C", raiz, "ls-files", "-z", "--", "*.sh"], capture_output=True, check=False)
    if saida.returncode != 0:
        raise ValueError("git ls-files falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    return [nome for nome in saida.stdout.decode("utf-8", "replace").split("\0") if nome]


def verificar(raiz):
    nomes = []
    run = build = excecoes = lacunas = 0
    violacoes = []
    try:
        nomes = arquivos_sh(raiz)
    except ValueError as erro:
        violacoes.append(str(erro))
    for nome in nomes:
        try:
            with open(os.path.join(raiz, nome), encoding="utf-8") as fonte:
                texto = fonte.read()
        except (OSError, UnicodeDecodeError) as erro:
            violacoes.append(f"{nome}: não lido ({erro})")
            continue
        r, b, v, e, lac = conferir_texto(nome, texto)
        run, build, excecoes, lacunas = run + r, build + b, excecoes + e, lacunas + lac
        violacoes += v
    if excecoes != EXCECOES_ESPERADAS and nomes:
        violacoes.append(f"exceções declaradas: {excecoes}, esperada {EXCECOES_ESPERADAS} ({EXCECAO_ARQUIVO}, {EXCECAO_TEXTO})")
    for violacao in violacoes:
        print(f"VIOLACAO {violacao}")
    print(f"arquivos={len(nomes)} run={run} build={build} sem_cota={len(violacoes)} excecoes={excecoes} lacunas={lacunas}")
    if not nomes or run + build == 0:
        print("REPROVA: varredura vazia")
        return 1
    if violacoes:
        print("REPROVA: contêiner sem a cota do líder")
        return 1
    print("PASSA")
    return 0


EXC = 'docker run --pull=never --rm --privileged -v x "$FLATHUB_KDE" cmd\n'
BOM_RUN = 'docker run --pull=never "${NL_COTAS_DOCKER[@]}" --rm img sh\n'
BUILD = "docker build -t x .\n"
CASOS = (
    ("positivo", {"a.sh": BOM_RUN, "tools/espelho/comum.sh": BUILD + BUILD, EXCECAO_ARQUIVO: EXC}, 0, "lacunas=2"),
    ("run_sem_cota", {"a.sh": "docker run --pull=never --rm img sh\n", EXCECAO_ARQUIVO: EXC}, 1, "a.sh:1: docker run"),
    ("build_fora_da_lacuna", {"a.sh": BOM_RUN + BUILD, EXCECAO_ARQUIVO: EXC}, 1, "a.sh:2: docker build"),
    ("build_acima_do_teto", {"a.sh": BOM_RUN, "tools/espelho/comum.sh": BUILD * 3, EXCECAO_ARQUIVO: EXC}, 1,
     "tools/espelho/comum.sh:3: docker build"),
    ("vetor_errado", {"a.sh": BOM_RUN.replace("NL_COTAS_DOCKER[@]", "OUTRO[@]"), EXCECAO_ARQUIVO: EXC}, 1, "a.sh:1:"),
    ("vetor_em_comentario", {"a.sh": 'docker run --rm img # "${NL_COTAS_DOCKER[@]}"\n', EXCECAO_ARQUIVO: EXC}, 1, "a.sh:1:"),
    ("vetor_em_outro_comando", {"a.sh": 'docker run --rm img && echo "${NL_COTAS_DOCKER[@]}"\n', EXCECAO_ARQUIVO: EXC}, 1,
     "a.sh:1:"),
    ("continuacao_com_vetor", {"a.sh": 'CONT=(docker run --pull=never \\\n  "${NL_COTAS_DOCKER[@]}" img)\n',
                               EXCECAO_ARQUIVO: EXC}, 0, "PASSA"),
    ("continuacao_sem_vetor", {"a.sh": "CONT=(docker run --pull=never \\\n  --rm img)\n", EXCECAO_ARQUIVO: EXC}, 1, "a.sh:1:"),
    ("chamada_em_aspas_fica_fora", {"a.sh": BOM_RUN + "grep '^docker run ' log\n", EXCECAO_ARQUIVO: EXC}, 0, "run=2"),
    ("excecao_fora_do_arquivo", {"a.sh": BOM_RUN + EXC, EXCECAO_ARQUIVO: EXC}, 1, "a.sh:2:"),
    ("excecao_ausente", {"a.sh": BOM_RUN}, 1, "exceções declaradas: 0"),
    ("vazia", {"leia.txt": "nada\n"}, 1, "varredura vazia"),
    ("sem_chamada", {"a.sh": "echo oi\n", EXCECAO_ARQUIVO: "echo oi\n"}, 1, "exceções declaradas: 0"),
    ("sem_git", None, 1, "git ls-files falhou"),
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


def autoteste():
    falhas = 0
    with tempfile.TemporaryDirectory(prefix="notalivre-cota-conteiner-") as base:
        os.environ["GIT_CEILING_DIRECTORIES"] = base
        for nome, arquivos, rc_esperado, trecho in CASOS:
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
                texto = entrada.read()
            certo = rc == rc_esperado and trecho in texto
            print(f"--- caso {nome}: rc={rc} (esperado {rc_esperado}) {'OK' if certo else 'FALHOU'}")
            if not certo:
                falhas += 1
                print(texto)
    print(f"casos={len(CASOS)} falharam={falhas}")
    return 1 if falhas or not CASOS else 0


def main(argv):
    if len(argv) == 2 and argv[1] == "--autoteste":
        return autoteste()
    if len(argv) == 2:
        return verificar(argv[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
