#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão de higiene do NotaLivre (NL-0.6): travessão (U+2014) e meia-risca (U+2013).

Grupos varridos:
  UI   : .qml e .ts (conteúdo inteiro); .cpp e .h (somente o texto dentro de tr("...") e qsTr("...")).
  DOCS : README.md da raiz e docs/**/*.md voltados ao usuário. Excluídos (documentação interna
         de desenvolvimento): docs/superpowers/, docs/pesquisa/, docs/design/, docs/origem/, docs/auditoria/,
         docs/adr/ e TODO.md.

Allowlist ancorada na raiz: o caminho relativo que COMEÇA por tests/fixtures/higiene/
(prefixo exato, não substring). Esses arquivos contam em ignorados_allowlist.

Pisos: o grupo DOCS exige encontrados_docs > 0 sempre. O piso do grupo UI vem de
travessao_config.json (piso_ui_ligado, desligado até a fatia NL-1.8). A opção --piso-ui=on|off
sobrepõe a configuração e existe para o autoteste.

Uso: python3 -I higiene_travessao.py [--piso-ui=on|off] RAIZ [RAIZ ...]

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M violacoes=K".
Sai com 1 se houver violação, piso DOCS zerado, piso UI ligado e zerado, arquivo encontrado
e não analisado, ou configuração ausente ou inválida (fail-closed).
"""

import json
import os
import re
import sys

DIR_PORTOES = os.path.dirname(os.path.abspath(__file__))
CONFIG = os.path.join(DIR_PORTOES, "travessao_config.json")
PROIBIDOS = {
    b"\xe2\x80\x94": "travessao U+2014",
    b"\xe2\x80\x93": "meia-risca U+2013",
}
PREFIXO_ALLOWLIST = "tests/fixtures/higiene/"
DIRETORIOS_IGNORADOS = (".git",)
EXT_UI_INTEIRO = (".qml", ".ts")
EXT_UI_LITERAL = (".cpp", ".h")
# Documentação interna, fora do grupo DOCS (decisão do orquestrador, 25/10/2026): desenvolvimento (superpowers,
# pesquisa, design), brief de origem de terceiro (docs/origem: texto de origem fica, exceção da L-32),
# auditoria e ADRs.
DOCS_EXCLUIDOS = ("docs/superpowers/", "docs/pesquisa/", "docs/design/", "docs/origem/", "docs/auditoria/", "docs/adr/")
RE_TR = re.compile(rb'\b(?:tr|qsTr)\(\s*"((?:[^"\\]|\\.)*)"')


def classificar(rel):
    """Devolve (grupo, so_literal) ou None se o arquivo fica fora dos grupos."""
    ext = os.path.splitext(rel)[1]
    if ext in EXT_UI_INTEIRO:
        return ("UI", False)
    if ext in EXT_UI_LITERAL:
        return ("UI", True)
    if rel == "README.md":
        return ("DOCS", False)
    if rel.startswith("docs/") and ext == ".md" and not rel.startswith(DOCS_EXCLUIDOS):
        return ("DOCS", False)
    return None


def coletar(raiz):
    """Devolve (elegiveis, ignorados). elegiveis = [(caminho, rel, grupo, so_literal)]."""
    elegiveis = []
    ignorados = 0
    for atual, dirs, arquivos in os.walk(raiz, followlinks=False):
        dirs[:] = [d for d in dirs if d not in DIRETORIOS_IGNORADOS]
        for nome in arquivos:
            caminho = os.path.join(atual, nome)
            rel = os.path.relpath(caminho, raiz).replace(os.sep, "/")
            if rel.startswith(PREFIXO_ALLOWLIST):
                if classificar(rel) is not None:
                    ignorados += 1
                continue
            classe = classificar(rel)
            if classe is not None:
                elegiveis.append((caminho, rel, classe[0], classe[1]))
    return elegiveis, ignorados


def trechos_verificados(dados, so_literal):
    """Devolve [(deslocamento, bytes)] a verificar: o arquivo inteiro, ou só tr/qsTr."""
    if not so_literal:
        return [(0, dados)]
    return [(m.start(1), m.group(1)) for m in RE_TR.finditer(dados)]


def analisar(elegiveis):
    """Devolve (analisados, violacoes). Imprime cada violação com arquivo:linha."""
    analisados = 0
    violacoes = []
    for caminho, rel, _grupo, so_literal in elegiveis:
        try:
            with open(caminho, "rb") as arquivo:
                dados = arquivo.read()
        except OSError as erro:
            print(f"NAO-ANALISADO {rel}: {erro.strerror}")
            continue
        analisados += 1
        for deslocamento, trecho in trechos_verificados(dados, so_literal):
            for sequencia, nome in PROIBIDOS.items():
                pos = trecho.find(sequencia)
                while pos != -1:
                    linha = dados.count(b"\n", 0, deslocamento + pos) + 1
                    violacoes.append((rel, linha, nome))
                    pos = trecho.find(sequencia, pos + 1)
    for rel, linha, nome in sorted(violacoes):
        print(f"VIOLACAO {rel}:{linha}: {nome}")
    return analisados, violacoes


def ler_piso_ui_configurado():
    """Devolve (piso_ui_ligado, erro). Erro preenchido = configuração ausente ou inválida."""
    try:
        with open(CONFIG, "rb") as arquivo:
            dados = json.loads(arquivo.read().decode("utf-8"))
        valor = dados["piso_ui_ligado"]
        if not isinstance(valor, bool):
            return False, "piso_ui_ligado não é booleano"
        return valor, None
    except (OSError, ValueError, KeyError) as erro:
        return False, f"{CONFIG}: {erro}"


def main(argv):
    piso_forcado = None
    raizes = []
    for arg in argv:
        if arg.startswith("--piso-ui="):
            valor = arg.split("=", 1)[1]
            if valor not in ("on", "off"):
                print("uso: --piso-ui=on|off")
                return 1
            piso_forcado = valor == "on"
        else:
            raizes.append(arg)

    piso_ui, erro_config = ler_piso_ui_configurado()
    if piso_forcado is not None:
        piso_ui = piso_forcado

    enc_total = enc_ui = enc_docs = 0
    ana_total = vio_total = ignorados = 0
    for raiz in raizes:
        if not os.path.isdir(raiz):
            print(f"RAIZ-INEXISTENTE {raiz}")
            continue
        elegiveis, ign = coletar(raiz)
        ana, viol = analisar(elegiveis)
        enc_total += len(elegiveis)
        enc_ui += sum(1 for item in elegiveis if item[2] == "UI")
        enc_docs += sum(1 for item in elegiveis if item[2] == "DOCS")
        ana_total += ana
        vio_total += len(viol)
        ignorados += ign

    print(f"encontrados={enc_total} analisados={ana_total} violacoes={vio_total}")
    print(f"encontrados_ui={enc_ui} encontrados_docs={enc_docs} "
          f"ignorados_allowlist={ignorados} piso_ui_ligado={piso_ui}")

    if erro_config is not None:
        print(f"REPROVA: configuração ausente ou inválida ({erro_config})")
        return 1
    if vio_total > 0:
        print("REPROVA: travessao ou meia-risca em texto de interface ou documento")
        return 1
    if enc_docs == 0:
        print("REPROVA: piso DOCS (zero README.md ou docs/*.md voltados ao usuário)")
        return 1
    if piso_ui and enc_ui == 0:
        print("REPROVA: piso UI ligado e zero arquivos de interface (.qml, .ts, tr/qsTr)")
        return 1
    if ana_total != enc_total:
        print("REPROVA: arquivo encontrado e não analisado")
        return 1
    print("PASSA")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
