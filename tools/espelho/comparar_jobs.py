#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Comparador do espelho local com o CI (R2, achado A0-02; L-36). API pública do GitHub, sem token.

Uso: python3 -I comparar_jobs.py RAIZ [RUN_ID]
Compara três conjuntos de nomes de job:
  ci    : jobs de .github/workflows/ci.yml, com a matriz expandida (nativo x4);
  lista : primeira coluna de tools/espelho_local.txt;
  run   : jobs do RUN_ID, ou do último run concluído da branch onda-0 (o último empurrado).
Saída: "encontrados=N lista=L run=R ..." e "batem=B faltam=F", com a linha de cada nome faltante.
Sai com 1 se algum nome faltar em um dos três conjuntos, se o ci.yml tiver zero jobs (varredura vazia),
se a API falhar, ou se a lista de jobs do run vier paginada (incompleta).
"""

import json
import os
import re
import sys
import urllib.request

REPO = "petrinhu/notalivre"
BRANCH = "onda-0"
RE_JOB = re.compile(r"^  ([A-Za-z0-9_-]+):\s*$")
RE_NOME = re.compile(r"^    name:\s*(.+?)\s*$")
RE_DISTRO = re.compile(r"^\s+- distro:\s*(\S+)\s*$")
RE_TEMPLATE_DISTRO = re.compile(r"\$\{\{\s*matrix\.distro\s*\}\}")


def nomes_do_ci(texto):
    """Nomes dos jobs como o GitHub mostra: `name:` do job, com a matriz de distro expandida."""
    jobs = []
    em_jobs = False
    atual = None
    for linha in texto.splitlines():
        if linha.rstrip() == "jobs:":
            em_jobs = True
            continue
        if not em_jobs:
            continue
        casou = RE_JOB.match(linha)
        if casou:
            atual = {"id": casou.group(1), "nome": None, "distros": []}
            jobs.append(atual)
            continue
        if atual is None:
            continue
        casou = RE_NOME.match(linha)
        if casou and atual["nome"] is None:
            atual["nome"] = casou.group(1)
        casou = RE_DISTRO.match(linha)
        if casou:
            atual["distros"].append(casou.group(1))
    nomes = []
    for job in jobs:
        modelo = job["nome"] or job["id"]
        if job["distros"] and RE_TEMPLATE_DISTRO.search(modelo):
            nomes.extend(RE_TEMPLATE_DISTRO.sub(d, modelo) for d in job["distros"])
        else:
            nomes.append(modelo)
    return nomes


def nomes_da_lista(texto):
    nomes = []
    for linha in texto.splitlines():
        if not linha.strip() or linha.lstrip().startswith("#"):
            continue
        if " | " not in linha:
            raise ValueError(f"linha sem separador ' | ': {linha[:60]}")
        nomes.append(linha.split(" | ", 1)[0].strip())
    return nomes


def api(url):
    pedido = urllib.request.Request(url, headers={
        "Accept": "application/vnd.github+json",
        "User-Agent": "notalivre-espelho-local",
    })
    with urllib.request.urlopen(pedido, timeout=30) as resposta:
        return json.load(resposta)


def run_e_jobs(run_id):
    if run_id:
        run = api(f"https://api.github.com/repos/{REPO}/actions/runs/{run_id}")
    else:
        lista = api(f"https://api.github.com/repos/{REPO}/actions/runs?branch={BRANCH}&per_page=10")
        concluidos = [r for r in lista.get("workflow_runs", []) if r.get("status") == "completed"]
        if not concluidos:
            raise RuntimeError(f"nenhum run concluído na branch {BRANCH}")
        run = concluidos[0]
    jobs = api(f"https://api.github.com/repos/{REPO}/actions/runs/{run['id']}/jobs?per_page=100")
    if jobs.get("total_count", 0) > len(jobs.get("jobs", [])):
        raise RuntimeError("lista de jobs do run paginada além de 100: incompleta")
    return run, [j["name"] for j in jobs.get("jobs", [])]


def main(argv):
    if not argv:
        print("uso: comparar_jobs.py RAIZ [RUN_ID]")
        return 1
    raiz = argv[0]
    run_id = argv[1] if len(argv) > 1 and argv[1] else None
    with open(os.path.join(raiz, ".github", "workflows", "ci.yml"), encoding="utf-8") as fonte:
        ci = nomes_do_ci(fonte.read())
    with open(os.path.join(raiz, "tools", "espelho_local.txt"), encoding="utf-8") as fonte:
        lista = nomes_da_lista(fonte.read())
    try:
        run, nomes_run = run_e_jobs(run_id)
    except (OSError, ValueError, RuntimeError, KeyError) as erro:
        print(f"REPROVA: API do GitHub indisponível ou incompleta ({erro})")
        return 1

    c_ci, c_lista, c_run = set(ci), set(lista), set(nomes_run)
    batem = sorted(c_ci & c_lista & c_run)
    faltam = sorted((c_ci | c_lista | c_run) - set(batem))
    print(f"encontrados={len(ci)} lista={len(lista)} run={len(nomes_run)} "
          f"run_id={run['id']} run_sha={run['head_sha'][:7]}")
    print(f"batem={len(batem)} faltam={len(faltam)}")
    for nome in faltam:
        print(f"FALTA \"{nome}\": ci={nome in c_ci} lista={nome in c_lista} run={nome in c_run}")
    if not ci:
        print("REPROVA: varredura vazia (zero jobs no ci.yml)")
        return 1
    if faltam:
        print("REPROVA: espelho, ci.yml e último run divergem")
        return 1
    print("PASSA: espelho, ci.yml e último run batem")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
