#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão de fixação do CI (R6, A0-07; controles da ADR 0001, L-77). Lê .github/workflows/ci.yml.

Verifica, em uma única leitura:
  - cada `uses:` aponta para commit SHA de 40 caracteres hexadecimais (nunca tag);
  - cada `image:` ou `imagem:` de container tem digest `@sha256:` de 64 caracteres hexadecimais;
  - `--privileged` aparece só no job `flatpak` (o único com risco aceito pela ADR 0001).

Uso: python3 -I fixacao_ci.py RAIZ            (verificação)
     python3 -I fixacao_ci.py --autoteste     (controles: positivo e três sabotagens)
Saída: "acoes=N imagens=M privilegiados=P violacoes=V". Sai com 1 se houver violação, se não houver
nenhuma ação nem imagem (varredura vazia), ou se o arquivo não existir (fail-closed).
"""

import os
import re
import sys
import tempfile

RE_USES = re.compile(r"^\s*(?:-\s+)?uses:\s*(\S+)")
RE_IMAGEM = re.compile(r"^\s*(?:image|imagem):\s*(\S+)")
RE_JOB = re.compile(r"^  ([A-Za-z0-9_-]+):\s*$")
RE_SHA_COMMIT = re.compile(r"^[0-9a-f]{40}$")
RE_DIGEST = re.compile(r"@sha256:[0-9a-f]{64}$")
RE_SEGREDO = re.compile(r"secrets\.")
RE_PERMISSAO_ESCRITA = re.compile(r"(id-token|packages|contents|pull-requests):\s*write")
RE_RUNS_ON = re.compile(r"^    runs-on:\s*(\S+)")
JOB_PRIVILEGIADO = "flatpak"
CAMINHO_CI = os.path.join(".github", "workflows", "ci.yml")


def verificar(texto):
    """Devolve (acoes, imagens, privilegiados, violacoes: lista de (linha, mensagem))."""
    acoes = imagens = 0
    privilegiados = {}
    violacoes = []
    job_atual = None
    dentro_de_jobs = False
    for numero, linha in enumerate(texto.splitlines(), 1):
        if linha.rstrip() == "jobs:":
            dentro_de_jobs = True
            continue
        if dentro_de_jobs:
            casou_job = RE_JOB.match(linha)
            if casou_job:
                job_atual = casou_job.group(1)
        casou = RE_USES.match(linha)
        if casou:
            acoes += 1
            alvo = casou.group(1)
            ref = alvo.rsplit("@", 1)[1] if "@" in alvo else ""
            if not RE_SHA_COMMIT.match(ref):
                violacoes.append((numero, f"action sem SHA de commit: {alvo}"))
        casou = RE_IMAGEM.match(linha)
        if casou and not casou.group(1).startswith("${{"):
            imagens += 1
            if not RE_DIGEST.search(casou.group(1)):
                violacoes.append((numero, f"imagem sem digest @sha256: {casou.group(1)}"))
        if RE_SEGREDO.search(linha):
            violacoes.append((numero, "uso de secrets no CI (ADR 0001: o job privilegiado não recebe segredo)"))
        if RE_PERMISSAO_ESCRITA.search(linha):
            violacoes.append((numero, f"permissão de escrita no CI: {linha.strip()}"))
        if "self-hosted" in linha:
            violacoes.append((numero, "runner self-hosted (ADR 0001 exige runner hospedado e efêmero)"))
        casou = RE_RUNS_ON.match(linha)
        if casou and job_atual == JOB_PRIVILEGIADO and casou.group(1) != "ubuntu-latest":
            violacoes.append((numero, f"job privilegiado em runner {casou.group(1)} (esperado ubuntu-latest, efêmero)"))
        if "--privileged" in linha:
            privilegiados[job_atual] = privilegiados.get(job_atual, 0) + 1
            if job_atual != JOB_PRIVILEGIADO:
                violacoes.append((numero, f"--privileged fora do job {JOB_PRIVILEGIADO} (job {job_atual})"))
    return acoes, imagens, sum(privilegiados.values()), violacoes


def verificar_arquivo(caminho):
    if not os.path.isfile(caminho):
        print(f"REPROVA: {caminho} não existe (fail-closed)")
        return 1
    with open(caminho, "r", encoding="utf-8") as fonte:
        texto = fonte.read()
    acoes, imagens, privilegiados, violacoes = verificar(texto)
    for numero, mensagem in violacoes:
        print(f"VIOLACAO {CAMINHO_CI}:{numero}: {mensagem}")
    print(f"acoes={acoes} imagens={imagens} privilegiados={privilegiados} violacoes={len(violacoes)}")
    if acoes + imagens == 0:
        print("REPROVA: varredura vazia (nenhuma action nem imagem encontrada)")
        return 1
    if violacoes:
        print("REPROVA: fixação do CI fora da ADR 0001")
        return 1
    print("PASSA")
    return 0


BOM = """jobs:
  a:
    runs-on: x
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
  flatpak:
    container:
      image: ghcr.io/x/y@sha256:""" + "b" * 64 + """
      options: --privileged
"""


def autoteste():
    """Controles: positivo passa; tag em action, tag em imagem e --privileged fora do flatpak reprovam."""
    casos = [
        ("positivo", BOM, 0),
        ("action-por-tag", BOM.replace("3d3c42e5aac5ba805825da76410c181273ba90b1", "v7"), 1),
        ("imagem-por-tag", BOM.replace("@sha256:" + "b" * 64, ":kde-6.11"), 1),
        ("segredo-no-ci", BOM.replace("      options: --privileged", "      options: --privileged\n    env:\n      T: ${{ secrets.TOKEN }}"), 1),
        ("runner-self-hosted", BOM.replace("  flatpak:\n", "  flatpak:\n    runs-on: self-hosted\n"), 1),
        ("privilegiado-fora-do-flatpak", BOM.replace("  a:\n    runs-on: x", "  a:\n    runs-on: x\n    container:\n      options: --privileged"), 1),
    ]
    falhas = 0
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR", "/var/tmp")) as tmp:
        for nome, texto, esperado in casos:
            pasta = os.path.join(tmp, nome)
            os.makedirs(os.path.join(pasta, ".github", "workflows"))
            with open(os.path.join(pasta, CAMINHO_CI), "w", encoding="utf-8") as destino:
                destino.write(texto)
            rc = verificar_arquivo(os.path.join(pasta, CAMINHO_CI))
            ok = rc == esperado
            falhas += 0 if ok else 1
            print(f"[{'VERDE' if ok else 'VERMELHO'}] {nome}: rc={rc} (esperado {esperado})")
    print(f"AUTOTESTE FIXACAO: falhas={falhas}")
    return 0 if falhas == 0 else 1


def main(argv):
    if argv == ["--autoteste"]:
        return autoteste()
    if len(argv) != 1:
        print("uso: fixacao_ci.py RAIZ | --autoteste")
        return 1
    return verificar_arquivo(os.path.join(argv[0], CAMINHO_CI))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
