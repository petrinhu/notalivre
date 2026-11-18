#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão do teto de processos em todo docker run (NL-3.1h, emenda v3.11; L-36, L-17).

Cada docker run de todo arquivo .sh versionado tem exatamente UMA fonte de teto de processos:
  - o vetor "${NL_COTAS_DOCKER[@]}", que já leva --pids-limit (400 por padrão, NL_PIDS); ou
  - o vetor "${NL_COTAS_DOCKER_SEM_PIDS[@]}" mais um --pids-limit literal de 1 a 400 (banco); ou
  - um --pids-limit literal de 1 a 400, sem vetor de cota de processos (ex.: exceção do Flathub).
Reprova: zero fontes; duas fontes; dois --pids-limit no mesmo docker run; valor fora de 1 a 400; vetor SEM_PIDS sem literal;
e varredura vazia (piso, L-36). O docker build não entra: o teto de processos é só do docker run.
A varredura reaproveita a do portão cota_em_todo_conteiner.py (strings entre aspas e comentários não contam).
Uso: python3 -I pids_em_docker_run.py RAIZ        (verificação; RAIZ precisa de .git)
     python3 -I pids_em_docker_run.py --autoteste (casos de fixture)
Rc: 0 passa; 1 reprova; 2 uso.
"""

import os
import re
import sys
import tempfile
import subprocess

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cota_em_todo_conteiner import (  # noqa: E402  (reuso da varredura, regra de 3 com o portão de cota)
    RE_CHAMADA,
    RE_FIM,
    VETOR_RUN,
    VETOR_RUN_SEM_PIDS,
    comandos,
    sem_aspas_e_comentario,
)

MARCA_VETOR = "@@PIDS_VETOR@@"
MARCA_SEM_PIDS = "@@PIDS_SEM@@"
LIMITE = 400
# Piso da varredura real (L-36): enumerado em 18/11/2026 pelo grep de `docker run` de todo .sh versionado, sem os trechos
# entre aspas e os comentários. Se mudar, o autoteste reprova, e o piso sobe no mesmo diff com a enumeração na mensagem.
PISO = 21
RE_PIDS = re.compile(r"--pids-limit[ \t]+(\S+)")


def conferir_comando(trecho):
    """Devolve a lista de violações de um docker run (trecho depois de 'docker run', até o separador)."""
    tem_vetor = MARCA_VETOR in trecho
    tem_sem = MARCA_SEM_PIDS in trecho
    literais = RE_PIDS.findall(trecho)
    violacoes = []
    if tem_vetor and (tem_sem or literais):
        violacoes.append("duas fontes de teto de processos (vetor com --pids-limit e mais uma fonte)")
    if tem_sem and not literais:
        violacoes.append("vetor NL_COTAS_DOCKER_SEM_PIDS sem --pids-limit literal")
    if len(literais) > 1:
        violacoes.append(f"{len(literais)} --pids-limit no mesmo docker run")
    for valor in literais:
        if not re.fullmatch(r"[1-9][0-9]{0,2}", valor) or int(valor) > LIMITE:
            violacoes.append(f"--pids-limit {valor} fora de 1 a {LIMITE}")
    if not tem_vetor and not tem_sem and not literais:
        violacoes.append("docker run sem teto de processos (vetor NL_COTAS_DOCKER ou --pids-limit)")
    return violacoes


def conferir_texto(nome, texto):
    """Devolve (encontrados, violações) de um arquivo. Só docker run de fora de comentário e de aspas."""
    encontrados = 0
    violacoes = []
    for numero, comando in comandos(texto):
        if comando.lstrip().startswith("#"):
            continue
        marcado = comando.replace(VETOR_RUN, MARCA_VETOR).replace(VETOR_RUN_SEM_PIDS, MARCA_SEM_PIDS)
        limpo = sem_aspas_e_comentario(marcado)
        for casou in RE_CHAMADA.finditer(limpo):
            if casou.group(1) != "run":
                continue
            encontrados += 1
            resto = limpo[casou.end():]
            fim = RE_FIM.search(resto)
            trecho = resto[: fim.start()] if fim else resto
            for motivo in conferir_comando(trecho):
                violacoes.append(f"{nome}:{numero}: {motivo}")
    return encontrados, violacoes


def arquivos_sh(raiz):
    saida = subprocess.run(["git", "-C", raiz, "ls-files", "-z", "--", "*.sh"], capture_output=True, check=False)
    if saida.returncode != 0:
        raise ValueError("git ls-files falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    return [nome for nome in saida.stdout.decode("utf-8", "replace").split("\0") if nome]


def verificar(raiz, piso=PISO):
    nomes = []
    encontrados = 0
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
        n, v = conferir_texto(nome, texto)
        encontrados += n
        violacoes += v
    if nomes and encontrados < piso:
        violacoes.append(f"abaixo do piso de varredura: encontrados={encontrados} (piso {piso})")
    for violacao in violacoes:
        print(f"VIOLACAO {violacao}")
    print(f"encontrados={encontrados} analisados={encontrados} falharam={len(violacoes)} piso={piso} arquivos={len(nomes)}")
    if not nomes or encontrados == 0:
        print("REPROVA: varredura vazia")
        return 1
    if violacoes:
        print("REPROVA: docker run sem teto de processos único")
        return 1
    print("PASSA")
    return 0


BOM_VETOR = 'docker run --pull=never "${NL_COTAS_DOCKER[@]}" --rm img sh\n'
BOM_SEM = 'docker run --pull=never "${NL_COTAS_DOCKER_SEM_PIDS[@]}" --pids-limit 400 --rm img sh\n'
BOM_LITERAL = "docker run --pull=never --pids-limit 128 --rm img sh\n"
CASOS = (
    ("positivo_vetor", {"a.sh": BOM_VETOR}, 0, "PASSA"),
    ("positivo_sem_pids_com_literal", {"a.sh": BOM_SEM}, 0, "PASSA"),
    ("positivo_literal_sem_vetor", {"a.sh": BOM_LITERAL}, 0, "PASSA"),
    ("sem_fonte", {"a.sh": "docker run --pull=never --rm img sh\n"}, 1, "sem teto de processos"),
    ("duas_fontes_vetor_e_literal", {"a.sh": 'docker run "${NL_COTAS_DOCKER[@]}" --pids-limit 100 img\n'}, 1,
     "duas fontes de teto"),
    ("duas_fontes_vetor_e_sem", {"a.sh": 'docker run "${NL_COTAS_DOCKER[@]}" "${NL_COTAS_DOCKER_SEM_PIDS[@]}" img\n'}, 1,
     "duas fontes de teto"),
    ("sem_pids_vetor_sem_literal", {"a.sh": 'docker run "${NL_COTAS_DOCKER_SEM_PIDS[@]}" img\n'}, 1,
     "sem --pids-limit literal"),
    ("literal_401", {"a.sh": "docker run --pids-limit 401 img\n"}, 1, "--pids-limit 401 fora de 1 a 400"),
    ("literal_512_fora_da_lista", {"a.sh": "docker run --pids-limit 512 img\n"}, 1, "--pids-limit 512 fora de 1 a 400"),
    ("literal_zero", {"a.sh": "docker run --pids-limit 0 img\n"}, 1, "--pids-limit 0 fora"),
    ("dois_literais", {"a.sh": "docker run --pids-limit 100 --pids-limit 200 img\n"}, 1, "2 --pids-limit"),
    ("comentario_nao_conta", {"a.sh": "# docker run img\n" + BOM_VETOR}, 0, "PASSA"),
    ("aspas_nao_contam", {"a.sh": 'echo "docker run sem nada"\n' + BOM_VETOR}, 0, "encontrados=1"),
    ("continuacao_conta", {"a.sh": 'CONT=(docker run --pull=never \\\n  "${NL_COTAS_DOCKER[@]}" img)\n'}, 0, "encontrados=1"),
    ("continuacao_sem_teto", {"a.sh": "CONT=(docker run --pull=never \\\n  --rm img)\n"}, 1, "sem teto de processos"),
    ("varredura_vazia", {"leia.txt": "nada\n"}, 1, "varredura vazia"),
    ("sem_git", None, 1, "git ls-files falhou"),
)


def montar(pasta, arquivos):
    os.makedirs(pasta, exist_ok=True)
    subprocess.run(["git", "-C", pasta, "init", "-q"], check=True)
    for nome, texto in arquivos.items():
        caminho = os.path.join(pasta, nome)
        os.makedirs(os.path.dirname(caminho) or pasta, exist_ok=True)
        with open(caminho, "w", encoding="utf-8") as arq:
            arq.write(texto)
    subprocess.run(["git", "-C", pasta, "add", "-A"], check=True)


def autoteste():
    falhas = 0
    with tempfile.TemporaryDirectory(prefix="notalivre-pids-") as base:
        for nome, arquivos, rc_esperado, trecho in CASOS:
            pasta = os.path.join(base, nome)
            if arquivos is None:
                pasta_sem_git = os.path.join(base, nome)
                os.makedirs(pasta_sem_git, exist_ok=True)
                saida = _capturar(pasta_sem_git)
                rc = saida[0]
                texto = saida[1]
            else:
                montar(pasta, arquivos)
                saida = _capturar(pasta)
                rc, texto = saida
            certo = rc == rc_esperado and trecho in texto
            print(f"--- caso {nome}: rc={rc} (esperado {rc_esperado}) {'OK' if certo else 'FALHOU'}")
            if not certo:
                falhas += 1
                print(texto)
    print(f"casos={len(CASOS)} falharam={falhas}")
    return 1 if falhas or not CASOS else 0


def _capturar(pasta):
    """Roda verificar() sobre pasta com a saída capturada; devolve (rc, texto)."""
    import io
    import contextlib

    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = verificar(pasta, piso=0)
    return rc, buf.getvalue()


def main(argv):
    if len(argv) == 2 and argv[1] == "--autoteste":
        return autoteste()
    if len(argv) == 2:
        return verificar(argv[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
