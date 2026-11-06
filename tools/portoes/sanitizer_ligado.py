#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão estático: o sanitizer ativo está ligado e ninguém o desliga (NL-3.0h; L-36). Lê a árvore rastreada, antes do build.
  1. o test preset asan-ubsan tem exatamente o ambiente fixado;
  2. o job asan do ci.yml cobre fedora45 e mint22.3, roda sanitizer_ativo.py e chama o ctest pelo preset;
  3. executar_no_conteiner.sh roda sanitizer_ativo.py;
  4. o autoteste de sanitizer_ativo.py está no job autotestes do ci.yml e em job_autotestes.sh;
  5. nenhum arquivo rastreado fora da lista usa um desligamento ou ajuste de sanitizer (.md é texto e fica fora);
  6. teste em tests/e2e não confere código de saída só por diferente de zero: o código próprio do sanitizer passaria.

Saída, sempre: "arquivos=N palavras=P violacoes=V". Zero arquivos reprova. Um único git ls-files, leitura em processo (L-11).

Uso: python3 -I sanitizer_ligado.py RAIZ        (verificação)
     python3 -I sanitizer_ligado.py --autoteste (controle positivo, uma sabotagem por regra e varredura vazia)
"""

import json
import os
import re
import subprocess
import sys
import tempfile

PRESET = "asan-ubsan"
AMBIENTE = {
    "ASAN_OPTIONS": "halt_on_error=1:detect_leaks=1:exitcode=86",
    "LSAN_OPTIONS": "detect_leaks=1:exitcode=86",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1:exitcode=86",
}
CI = ".github/workflows/ci.yml"
PRESETS = "CMakePresets.json"
EXECUTOR = "tools/conteiner/executar_no_conteiner.sh"
JOB_AUTOTESTES = "tools/espelho/job_autotestes.sh"
GATE = "tools/portoes/sanitizer_ativo.py"
AUTOTESTE_GATE = "tools/portoes/autoteste_sanitizer_ativo.sh"
PERMITIDOS = frozenset({PRESETS, GATE, "tools/portoes/sanitizer_ligado.py"})
RE_PALAVRAS = re.compile(
    rb"(?:ASAN|LSAN|UBSAN)_OPTIONS|detect_leaks|__(?:asan|lsan|ubsan)_default_(?:options|suppressions)"
    rb"|__lsan_(?:disable|enable|ignore_object)|__lsan::|no_sanitize|-fno-sanitize=|suppressions=")
RE_CODIGO_FROUXO = re.compile(rb"\bcodigo\s*!=\s*0\b|exitCode\(\)\s*!=\s*0")
RE_JOB = re.compile(r"^  ([A-Za-z0-9_-]+):\s*$")


def bloco_do_job(texto, job):
    linhas = []
    dentro = False
    for linha in texto.splitlines():
        casou = RE_JOB.match(linha)
        if casou:
            dentro = casou.group(1) == job
        if dentro:
            linhas.append(linha)
    return "\n".join(linhas)


def ler(raiz, relativo):
    try:
        with open(os.path.join(raiz, relativo), encoding="utf-8") as fonte:
            return fonte.read()
    except OSError:
        return ""


def conferir_ligacao(raiz):
    violacoes = []
    try:
        presets = json.loads(ler(raiz, PRESETS) or "{}")
    except json.JSONDecodeError:
        presets = {}
    ambiente = next((p.get("environment") for p in presets.get("testPresets", []) if p.get("name") == PRESET), None)
    if ambiente != AMBIENTE:
        violacoes.append(f"{PRESETS}: ambiente do test preset {PRESET} diferente do fixado: {ambiente}")
    ci = ler(raiz, CI)
    asan = bloco_do_job(ci, "asan")
    for trecho in (GATE, "distro: fedora45", "distro: mint22.3", f"ctest --preset {PRESET}"):
        if trecho not in asan:
            violacoes.append(f"{CI}: job asan sem [{trecho}]")
    if GATE not in ler(raiz, EXECUTOR):
        violacoes.append(f"{EXECUTOR}: não roda {GATE}")
    if AUTOTESTE_GATE not in bloco_do_job(ci, "autotestes"):
        violacoes.append(f"{CI}: job autotestes sem {AUTOTESTE_GATE}")
    if AUTOTESTE_GATE not in ler(raiz, JOB_AUTOTESTES):
        violacoes.append(f"{JOB_AUTOTESTES}: sem {AUTOTESTE_GATE}")
    return violacoes


def conferir_arvore(raiz):
    """Devolve (arquivos, palavras, violações)."""
    saida = subprocess.run(["git", "-C", raiz, "ls-files", "-z"], capture_output=True, check=False)
    if saida.returncode != 0:
        raise RuntimeError("git ls-files falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    arquivos = palavras = 0
    violacoes = []
    for relativo in (c.decode("utf-8", "replace") for c in saida.stdout.split(b"\0") if c):
        caminho = os.path.join(raiz, relativo)
        if not os.path.isfile(caminho) or os.path.islink(caminho):
            continue
        arquivos += 1
        with open(caminho, "rb") as fonte:
            conteudo = fonte.read()
        if relativo not in PERMITIDOS and not relativo.endswith(".md"):
            for casou in RE_PALAVRAS.finditer(conteudo):
                palavras += 1
                violacoes.append(f"{relativo}: [{casou.group(0).decode()}] fora da lista: ajuste de sanitizer só no preset")
        if relativo.startswith("tests/e2e/") and RE_CODIGO_FROUXO.search(conteudo):
            violacoes.append(f"{relativo}: código de saída conferido só por diferente de zero; use o código exato")
    return arquivos, palavras, violacoes


def verificar(raiz):
    try:
        arquivos, palavras, violacoes = conferir_arvore(raiz)
    except (RuntimeError, OSError) as erro:
        print(f"REPROVA: {erro}")
        return 1
    violacoes += conferir_ligacao(raiz)
    for violacao in violacoes:
        print(f"VIOLACAO {violacao}")
    print(f"arquivos={arquivos} palavras={palavras} violacoes={len(violacoes)}")
    if arquivos == 0:
        print("REPROVA: varredura vazia")
        return 1
    if violacoes:
        print("REPROVA: sanitizer desligado ou fora do job")
        return 1
    print("PASSA")
    return 0


CI_BOM = f"""jobs:
  asan:
    strategy:
      matrix:
        include:
          - distro: fedora45
          - distro: mint22.3
    steps:
      - run: python3 -I {GATE} x y
      - run: ctest --preset {PRESET} --output-on-failure
  autotestes:
    steps:
      - run: bash {AUTOTESTE_GATE}
"""
ARVORE_BOA = {
    PRESETS: json.dumps({"testPresets": [{"name": PRESET, "environment": AMBIENTE}]}),
    CI: CI_BOM,
    EXECUTOR: f"python3 -I {GATE} a b\n",
    JOB_AUTOTESTES: f"bash {AUTOTESTE_GATE}\n",
    "docs/x.md": "LSAN_OPTIONS em texto\n",
    "tests/e2e/a.cpp": "QVERIFY(execucao.codigo == 4);\n",
}


def autoteste():
    def sem(chave, velho, novo):
        return {chave: ARVORE_BOA[chave].replace(velho, novo)}

    casos = (
        ("positivo", {}, 0, "PASSA"),
        ("preset_sem_pino", {PRESETS: json.dumps({"testPresets": [{"name": PRESET, "environment": {"ASAN_OPTIONS": "x"}}]})}, 1,
         "diferente do fixado"),
        ("ci_sem_gate", sem(CI, f"python3 -I {GATE} x y", "true"), 1, "job asan sem"),
        ("ci_sem_mint", sem(CI, "          - distro: mint22.3\n", ""), 1, "job asan sem"),
        ("ci_ctest_sem_preset", sem(CI, f"ctest --preset {PRESET}", "ctest"), 1, "job asan sem"),
        ("executor_sem_gate", {EXECUTOR: "true\n"}, 1, "não roda"),
        ("ci_sem_autoteste", sem(CI, f"bash {AUTOTESTE_GATE}", "true"), 1, "job autotestes sem"),
        ("espelho_sem_autoteste", {JOB_AUTOTESTES: "true\n"}, 1, "sem tools/portoes/autoteste"),
        ("desliga_no_ci", sem(CI, "steps:\n      - run: python3", "env:\n      LSAN_OPTIONS: detect_leaks=0\n    steps:\n      - run: python3"), 1,
         "fora da lista"),
        ("desliga_em_codigo", {"src/a.cpp": "void f() { __lsan_disable(); }\n"}, 1, "fora da lista"),
        ("supressao_em_teste", {"tests/a.txt": "suppressions=a.supp\n"}, 1, "fora da lista"),
        ("flag_por_alvo", {"apps/CMakeLists.txt": "target_compile_options(a PRIVATE -fno-sanitize=all)\n"}, 1, "fora da lista"),
        ("codigo_frouxo", {"tests/e2e/a.cpp": "QVERIFY(execucao.codigo != 0);\n"}, 1, "só por diferente de zero"),
        ("vazia", None, 1, "varredura vazia"),
    )
    falhas = 0
    with tempfile.TemporaryDirectory(prefix="notalivre-sanitizer-ligado-") as base:
        for nome, troca, rc_esperado, trecho in casos:
            raiz = os.path.join(base, nome)
            os.makedirs(raiz)
            subprocess.run(["git", "-C", raiz, "init", "-q"], check=True)
            if troca is not None:
                for relativo, conteudo in {**ARVORE_BOA, **troca}.items():
                    caminho = os.path.join(raiz, relativo)
                    os.makedirs(os.path.dirname(caminho) or raiz, exist_ok=True)
                    with open(caminho, "w", encoding="utf-8") as saida:
                        saida.write(conteudo)
                subprocess.run(["git", "-C", raiz, "add", "-A"], check=True)
            arquivo = os.path.join(base, nome + ".txt")
            with open(arquivo, "w", encoding="utf-8") as saida:
                antigo = sys.stdout
                sys.stdout = saida
                try:
                    rc = verificar(raiz)
                finally:
                    sys.stdout = antigo
            with open(arquivo, encoding="utf-8") as entrada:
                texto = entrada.read()
            certo = rc == rc_esperado and trecho in texto
            print(f"--- caso {nome}: rc={rc} (esperado {rc_esperado}) {'OK' if certo else 'FALHOU'}")
            if not certo:
                falhas += 1
                print(texto)
    print(f"casos={len(casos)} falharam={falhas}")
    return 1 if falhas else 0


def main(argv):
    if len(argv) == 2 and argv[1] == "--autoteste":
        return autoteste()
    if len(argv) == 2:
        return verificar(argv[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
