#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão do sanitizer ativo (NL-3.0h; L-36). Roda no build do preset asan-ubsan, antes do ctest, e prova pelo objeto:
  1. todo teste com PASS_REGULAR_EXPRESSION carrega, por igualdade, os regex do gate; WILL_FAIL e SKIP_* não existem;
  2. todo executável de teste do build, a CLI e a GUI são ELF com o símbolo __asan_init;
  3. o canário, com o ambiente do test preset: leak, heap e ub dão relatório e saem com o código próprio; ok sai limpo;
  4. o CTest real reprova o canário que vaza quando os regex do gate estão lá, aprova o limpo e, sem os regex, deixa o
     vazamento passar (é o furo que o gate fecha);
  5. guarda cruzada: sanitizer_ligado.py continua na lista de portões e no job higiene. Portão retirado de lá não roda e não
     avisa; quem avisa é este, que roda em outro job. O inverso está em sanitizer_ligado.py. Retirar os dois no mesmo commit
     nenhum portão pega: é risco aceito, só a revisão do diff vê.

Saída, sempre: "testes=N com_pass=P binarios=B canarios=C violacoes=V". Zero testes, zero com PASS ou zero binários reprova.
Um único processo por etapa, nenhum por arquivo (L-11).

Uso: python3 -I sanitizer_ativo.py BUILD PRESETS   (BUILD: pasta de build do preset; PRESETS: CMakePresets.json)
     python3 -I sanitizer_ativo.py --autoteste     (controle positivo, sabotagens e varreduras vazias, com o ctest real)
Rc: 0 passa; 1 reprova; 2 uso.
"""

import json
import mmap
import os
import re
import subprocess
import sys
import tempfile

PRESET = "asan-ubsan"
CODIGO_SANITIZER = 86
REGEX_GATE = ("(Leak|Address)Sanitizer", "runtime error:")
MARCA = "CANARIO-PASS"
PROPRIEDADES_PROIBIDAS = ("WILL_FAIL", "SKIP_RETURN_CODE", "SKIP_REGULAR_EXPRESSION")
SIMBOLO = b"__asan_init"
LISTA_PORTOES = "tools/conteiner/lista_portoes.txt"
CI = ".github/workflows/ci.yml"
COMANDO_LIGADO = "python3 -I /src/tools/portoes/sanitizer_ligado.py /src && python3 -I /src/tools/portoes/sanitizer_ligado.py --autoteste"
LINHA_LIGADO = "sanitizer_ligado|git config --global --add safe.directory /src && " + COMANDO_LIGADO
CASOS_CANARIO = (
    ("leak", "ERROR: LeakSanitizer"),
    ("heap", "ERROR: AddressSanitizer: heap-buffer-overflow"),
    ("ub", "runtime error:"),
)


def ambiente_do_preset(caminho):
    """Devolve o environment do test preset. Falta de arquivo, de preset ou de ambiente levanta ValueError (fail-closed)."""
    try:
        with open(caminho, encoding="utf-8") as fonte:
            presets = json.load(fonte)
    except (OSError, json.JSONDecodeError) as erro:
        raise ValueError(f"presets ilegíveis: {erro}") from erro
    for preset in presets.get("testPresets", []):
        if preset.get("name") == PRESET and preset.get("environment"):
            return preset["environment"]
    raise ValueError(f"test preset {PRESET} sem environment")


def listar_testes(build, ambiente):
    saida = subprocess.run(["ctest", "--test-dir", build, "--show-only=json-v1"], capture_output=True, env=ambiente,
                           check=False)
    if saida.returncode != 0:
        raise ValueError("ctest --show-only falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    return json.loads(saida.stdout.decode("utf-8", "replace")).get("tests", [])


def conferir_testes(testes):
    """Devolve (quantos têm PASS, violações)."""
    com_pass = 0
    violacoes = []
    for teste in testes:
        nome = teste.get("name", "?")
        propriedades = {p["name"]: p["value"] for p in teste.get("properties", [])}
        for proibida in PROPRIEDADES_PROIBIDAS:
            if propriedades.get(proibida) not in (None, False):
                violacoes.append(f"{nome}: {proibida} esconde o erro do sanitizer")
        if "PASS_REGULAR_EXPRESSION" not in propriedades:
            continue
        com_pass += 1
        falha = propriedades.get("FAIL_REGULAR_EXPRESSION", [])
        falha = [falha] if isinstance(falha, str) else falha
        for regex in REGEX_GATE:
            if regex not in falha:
                violacoes.append(f"{nome}: tem PASS e não tem o regex do gate [{regex}]")
    return com_pass, violacoes


def executaveis_dos_testes(build, testes):
    prefixo = os.path.abspath(build) + os.sep
    caminhos = set()
    for teste in testes:
        comando = teste.get("command") or []
        if comando and comando[0].startswith(prefixo):
            caminhos.add(comando[0])
    return sorted(caminhos)


def conferir_binarios(caminhos):
    violacoes = []
    for caminho in caminhos:
        try:
            with open(caminho, "rb") as binario:
                if binario.read(4) != b"\x7fELF":
                    violacoes.append(f"{caminho}: não é ELF")
                    continue
                with mmap.mmap(binario.fileno(), 0, access=mmap.ACCESS_READ) as conteudo:
                    if conteudo.find(SIMBOLO) < 0:
                        violacoes.append(f"{caminho}: sem {SIMBOLO.decode()} (sanitizer fora do binário)")
        except (OSError, ValueError) as erro:
            violacoes.append(f"{caminho}: não lido ({erro})")
    return violacoes


def rodar_canario(canario, modo, ambiente):
    """Devolve (rc, texto); rc None se não rodou."""
    try:
        saida = subprocess.run([canario, modo], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=ambiente,
                               timeout=120, check=False)
    except (OSError, subprocess.TimeoutExpired) as erro:
        return None, str(erro)
    return saida.returncode, saida.stdout.decode("utf-8", "replace")


def conferir_canario(canario, ambiente):
    violacoes = []
    rc, texto = rodar_canario(canario, "ok", ambiente)
    if rc != 0 or MARCA not in texto or any(re.search(r, texto) for r in REGEX_GATE):
        violacoes.append(f"canário ok: rc={rc}, esperado 0, com a marca e sem relatório. Saída: {texto[:400]}")
    for modo, trecho in CASOS_CANARIO:
        rc, texto = rodar_canario(canario, modo, ambiente)
        if rc != CODIGO_SANITIZER or trecho not in texto or not any(re.search(r, texto) for r in REGEX_GATE):
            violacoes.append(f"canário {modo}: rc={rc}, esperado {CODIGO_SANITIZER} com [{trecho}]. Saída: {texto[:400]}")
    return violacoes


def conferir_ctest(canario, pasta, ambiente):
    """O CTest real decide: é a regra que o gate usa, na versão do CTest do próprio job."""
    os.makedirs(pasta, exist_ok=True)
    falha = ";".join(REGEX_GATE)
    casos = (("canario_limpo", "ok", True, 0), ("canario_vazando", "leak", True, 1), ("canario_sem_gate", "leak", False, 0))
    linhas = []
    for nome, modo, com_gate, _ in casos:
        linhas.append(f'add_test([=[{nome}]=] "{canario}" "{modo}")')
        extra = f' FAIL_REGULAR_EXPRESSION "{falha}"' if com_gate else ""
        linhas.append(f'set_tests_properties([=[{nome}]=] PROPERTIES PASS_REGULAR_EXPRESSION "{MARCA}"{extra})')
    with open(os.path.join(pasta, "CTestTestfile.cmake"), "w", encoding="utf-8") as saida:
        saida.write("\n".join(linhas) + "\n")
    violacoes = []
    for nome, _, _, esperado in casos:
        rodada = subprocess.run(["ctest", "--test-dir", pasta, "--no-tests=error", "-R", f"^{nome}$"], capture_output=True,
                                env=ambiente, check=False)
        if (rodada.returncode != 0) != bool(esperado):
            texto = rodada.stdout.decode("utf-8", "replace")[-400:]
            violacoes.append(f"ctest {nome}: rc={rodada.returncode}, esperado {'reprovar' if esperado else 'passar'}. Saída: {texto}")
    return violacoes


def verificar(build, presets, canario=None, extras=None):
    build = os.path.abspath(build)
    canario = canario or os.path.join(build, "tests", "sanidade", "nl_canario_sanitizer")
    if extras is None:
        extras = [os.path.join(build, "apps", "cli", "notalivre-cli"), os.path.join(build, "apps", "gui", "notalivre-gui")]
    testes = []
    com_pass = 0
    binarios = []
    violacoes = []
    canarios = 0
    try:
        ambiente = {**os.environ, **ambiente_do_preset(presets)}
        testes = listar_testes(build, ambiente)
        com_pass, violacoes = conferir_testes(testes)
        de_testes = executaveis_dos_testes(build, testes)
        binarios = sorted(set(de_testes) | set(extras))
        violacoes += conferir_binarios(binarios)
        if not de_testes:
            violacoes.append("nenhum executável de teste dentro do build: varredura vazia")
        violacoes += conferir_canario(canario, ambiente)
        canarios = 1 + len(CASOS_CANARIO)
        violacoes += conferir_ctest(canario, os.path.join(build, "sanitizer_ativo"), ambiente)
    except (ValueError, OSError) as erro:
        violacoes.append(str(erro))
    for violacao in violacoes:
        print(f"VIOLACAO {violacao}")
    print(f"testes={len(testes)} com_pass={com_pass} binarios={len(binarios)} canarios={canarios} violacoes={len(violacoes)}")
    if not testes or not com_pass or not binarios:
        print("REPROVA: varredura vazia")
        return 1
    if violacoes:
        print("REPROVA: sanitizer ou gate fora do esperado")
        return 1
    print("PASSA")
    return 0


CANARIO_BOM = """#!/bin/sh
echo CANARIO-PASS
case "$1" in
  ok) exit 0 ;;
  leak) echo '==1==ERROR: LeakSanitizer: detected memory leaks' >&2; exit 86 ;;
  heap) echo '==1==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x1' >&2; exit 86 ;;
  ub) echo 'a.cpp:1:1: runtime error: signed integer overflow' >&2; exit 86 ;;
esac
exit 2
"""
LINHA_LEAK = "leak) echo '==1==ERROR: LeakSanitizer: detected memory leaks' >&2; exit 86 ;;"
LINHA_HEAP = "heap) echo '==1==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x1' >&2; exit 86 ;;"
LINHA_UB = "ub) echo 'a.cpp:1:1: runtime error: signed integer overflow' >&2; exit 86 ;;"
FALHA_BOA = ";".join(("FAIL!",) + REGEX_GATE)
PRESETS_BOM = {"testPresets": [{"name": PRESET, "environment": {"X_NL": "1"}}]}
LISTA_BOA = "# comentário\ncamadas|true\n" + LINHA_LIGADO + "\n"
CI_BOM = f"""jobs:
  asan:
    steps:
      - run: true
  higiene:
    steps:
      - run: sh -c '{COMANDO_LIGADO}'
  camadas:
    steps:
      - run: true
"""


def montar(pasta, falha=FALHA_BOA, extra_um="", com_pass=True, com_simbolo=True, sem_testes=False, canario_txt=CANARIO_BOM,
           presets=None, com_canario=True, lista=LISTA_BOA, ci=CI_BOM):
    """Build de brincadeira: dois testes, um com PASS. Nada aqui executa o executável de teste: o ctest só o lista."""
    os.makedirs(pasta)
    binario = os.path.join(pasta, "bin_teste")
    with open(binario, "wb") as saida:
        saida.write(b"\x7fELF" + (SIMBOLO if com_simbolo else b"") + b"\0" * 64)
    os.chmod(binario, 0o755)
    linhas = []
    if not sem_testes:
        linhas.append(f'add_test([=[um]=] "{binario}")')
        if com_pass:
            propriedades = f'PASS_REGULAR_EXPRESSION "Totals" FAIL_REGULAR_EXPRESSION "{falha}"{extra_um}'
            linhas.append(f"set_tests_properties([=[um]=] PROPERTIES {propriedades})")
        linhas.append(f'add_test([=[dois]=] "{binario}")')
    with open(os.path.join(pasta, "CTestTestfile.cmake"), "w", encoding="utf-8") as saida:
        saida.write("\n".join(linhas) + "\n")
    canario = os.path.join(pasta, "canario.sh")
    if com_canario:
        with open(canario, "w", encoding="utf-8") as saida:
            saida.write(canario_txt)
        os.chmod(canario, 0o755)
    for relativo, conteudo in ((LISTA_PORTOES, lista), (CI, ci)):
        caminho = os.path.join(pasta, relativo)
        os.makedirs(os.path.dirname(caminho), exist_ok=True)
        with open(caminho, "w", encoding="utf-8") as saida:
            saida.write(conteudo)
    caminho_presets = os.path.join(pasta, "presets.json")
    with open(caminho_presets, "w", encoding="utf-8") as saida:
        json.dump(PRESETS_BOM if presets is None else presets, saida)
    return caminho_presets, canario


def autoteste():
    casos = (
        ("positivo", {}, 0, "PASSA"),
        ("sem_regex_do_gate", {"falha": "FAIL!"}, 1, "não tem o regex do gate"),
        ("regex_errado", {"falha": "FAIL!;(Leak|Address)Sanitizr;runtime error:"}, 1, "não tem o regex do gate"),
        ("so_um_regex", {"falha": "FAIL!;(Leak|Address)Sanitizer"}, 1, "não tem o regex do gate"),
        ("will_fail", {"extra_um": ' WILL_FAIL "TRUE"'}, 1, "WILL_FAIL"),
        ("skip_return_code", {"extra_um": ' SKIP_RETURN_CODE "86"'}, 1, "SKIP_RETURN_CODE"),
        ("binario_sem_simbolo", {"com_simbolo": False}, 1, "sem __asan_init"),
        ("leak_sem_relatorio", {"canario_txt": CANARIO_BOM.replace(LINHA_LEAK, "leak) exit 0 ;;")}, 1, "canário leak"),
        ("leak_outro_codigo", {"canario_txt": CANARIO_BOM.replace(LINHA_LEAK, LINHA_LEAK.replace("exit 86", "exit 1"))}, 1,
         "canário leak"),
        ("heap_sem_relatorio", {"canario_txt": CANARIO_BOM.replace(LINHA_HEAP, "heap) exit 0 ;;")}, 1, "canário heap"),
        ("ub_sem_relatorio", {"canario_txt": CANARIO_BOM.replace(LINHA_UB, "ub) exit 0 ;;")}, 1, "canário ub"),
        ("ok_com_relatorio", {"canario_txt": CANARIO_BOM.replace("ok) exit 0 ;;", "ok) echo 'x runtime error: y' >&2; exit 0 ;;")},
         1, "canário ok"),
        ("canario_ausente", {"com_canario": False}, 1, "canário ok"),
        ("preset_sem_ambiente", {"presets": {"testPresets": [{"name": PRESET}]}}, 1, "sem environment"),
        ("vazia_sem_testes", {"sem_testes": True}, 1, "varredura vazia"),
        ("vazia_sem_pass", {"com_pass": False}, 1, "varredura vazia"),
        ("ligado_fora_da_lista", {"lista": "camadas|true\n"}, 1, "sem a linha exata de sanitizer_ligado"),
        ("ligado_neutro_na_lista", {"lista": "sanitizer_ligado|true\n"}, 1, "sem a linha exata de sanitizer_ligado"),
        ("ligado_fora_do_higiene", {"ci": CI_BOM.replace(COMANDO_LIGADO, "true")}, 1, "job higiene sem sanitizer_ligado"),
        ("ligado_so_em_outro_job", {"ci": CI_BOM.replace("  higiene:", "  outro:")}, 1, "job higiene sem sanitizer_ligado"),
        ("ligado_fora_dos_dois", {"lista": "camadas|true\n", "ci": CI_BOM.replace(COMANDO_LIGADO, "true")}, 1,
         "job higiene sem sanitizer_ligado"),
    )
    falhas = 0
    with tempfile.TemporaryDirectory(prefix="notalivre-sanitizer-ativo-") as base:
        for nome, opcoes, rc_esperado, trecho in casos:
            pasta = os.path.join(base, nome)
            presets, canario = montar(pasta, **opcoes)
            arquivo = os.path.join(base, nome + ".txt")
            with open(arquivo, "w", encoding="utf-8") as saida:
                antigo = sys.stdout
                sys.stdout = saida
                try:
                    rc = verificar(pasta, presets, canario=canario, extras=[])
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
    if len(argv) == 3:
        return verificar(argv[1], argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
