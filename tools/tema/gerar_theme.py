#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Gerador do tema do NotaLivre (NL-1.8): lê o bloco JSON da seção 3.4 da spec visual e grava
apps/gui/qml/Theme.qml (singleton, único lugar com hexadecimal do app) e tests/qml/dados/tokens.json
(cópia verbatim do bloco, lida pelos testes).

Uso: python3 -I gerar_theme.py [--raiz RAIZ] [--verificar]
  --raiz       raiz do repositório (padrão: dois níveis acima deste arquivo)
  --verificar  não grava; sai com 1 se algum arquivo gerado diferir do que está em disco (anti-deriva)

Saída: "gerar_theme: secao=3.4 cores=N fontes=N escala=N raio=N espaco=N modo=gravar|verificar" e
"arquivos: ..." com o estado de cada arquivo. Sai com 1 em qualquer erro de leitura ou de formato (fail-closed).
"""

import json
import os
import re
import sys

SPEC = os.path.join("docs", "design", "spec-visual.md")
SAIDA_THEME = os.path.join("apps", "gui", "qml", "Theme.qml")
SAIDA_TOKENS = os.path.join("tests", "qml", "dados", "tokens.json")
RE_HEX = re.compile(r"^#[0-9A-Fa-f]{6}$")
CHAVES_OBRIGATORIAS = ("claro", "escuro", "fontes", "escala", "espaco", "raio")


def extrair_bloco_3_4(texto):
    """Devolve (texto_bruto_do_bloco, dados) do primeiro bloco ```json depois do título '### 3.4'."""
    inicio = texto.find("### 3.4")
    if inicio < 0:
        raise ValueError("seção '### 3.4' não encontrada na spec visual")
    abre = texto.find("```json\n", inicio)
    if abre < 0:
        raise ValueError("bloco ```json não encontrado depois de '### 3.4'")
    corpo_ini = abre + len("```json\n")
    fecha = texto.find("\n```", corpo_ini)
    if fecha < 0:
        raise ValueError("bloco ```json sem fecho")
    bruto = texto[corpo_ini:fecha] + "\n"
    return bruto, json.loads(bruto)


def validar(dados):
    for chave in CHAVES_OBRIGATORIAS:
        if chave not in dados:
            raise ValueError("bloco sem a chave obrigatória: " + chave)
    if set(dados["claro"]) != set(dados["escuro"]):
        raise ValueError("claro e escuro têm conjuntos de tokens diferentes")
    for modo in ("claro", "escuro"):
        for nome, valor in dados[modo].items():
            if not RE_HEX.match(valor):
                raise ValueError("cor fora do formato #RRGGBB: %s.%s = %r" % (modo, nome, valor))


def capitalizar(texto):
    return texto[:1].upper() + texto[1:]


def nome_qml_da_cor(nome):
    """QML lê propriedade 'on<Maiúscula>' como handler de sinal; o token onBrand vira sobreBrand."""
    if len(nome) > 2 and nome.startswith("on") and nome[2].isupper():
        return "sobre" + nome[2:]
    return nome


def gerar_theme(dados):
    linhas = [
        "// SPDX-License-Identifier: GPL-3.0-or-later",
        "// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa",
        "// GERADO por tools/tema/gerar_theme.py a partir da spec visual, seção 3.4. Não editar à mão.",
        "// Único lugar do app com cor em hexadecimal (portão nenhum_hex_fora_do_theme).",
        "",
        "pragma Singleton",
        "import QtQuick",
        "",
        "QtObject {",
        "    property bool escuro: false",
        "",
    ]
    for nome, claro in dados["claro"].items():
        escuro = dados["escuro"][nome]
        linhas.append('    readonly property color %s: escuro ? "%s" : "%s"' % (
            nome_qml_da_cor(nome), escuro.upper(), claro.upper()))
    linhas.append("")
    for nome, valor in dados["fontes"].items():
        linhas.append('    readonly property string fonte%s: "%s"' % (capitalizar(nome), valor))
    for nome, valor in dados["escala"].items():
        linhas.append("    readonly property int tam%s: %d" % (capitalizar(nome), valor))
    for nome, valor in dados["raio"].items():
        linhas.append("    readonly property int raio%s: %d" % (capitalizar(nome), valor))
    linhas.append("    readonly property var espaco: [%s]" % ", ".join(str(v) for v in dados["espaco"]))
    linhas.append("}")
    return "\n".join(linhas) + "\n"


def comparar_ou_gravar(caminho, conteudo, verificar):
    atual = None
    if os.path.exists(caminho):
        with open(caminho, encoding="utf-8") as arq:
            atual = arq.read()
    igual = atual == conteudo
    if verificar:
        return "igual" if igual else "DIFERE"
    if not igual:
        os.makedirs(os.path.dirname(caminho), exist_ok=True)
        with open(caminho, "w", encoding="utf-8", newline="\n") as arq:
            arq.write(conteudo)
        return "gravado"
    return "sem_mudanca"


def main(argv):
    raiz = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    verificar = False
    i = 1
    while i < len(argv):
        if argv[i] == "--raiz":
            raiz = os.path.abspath(argv[i + 1])
            i += 2
        elif argv[i] == "--verificar":
            verificar = True
            i += 1
        else:
            print("uso: gerar_theme.py [--raiz RAIZ] [--verificar]", file=sys.stderr)
            return 2
    try:
        with open(os.path.join(raiz, SPEC), encoding="utf-8") as arq:
            texto = arq.read()
        bruto, dados = extrair_bloco_3_4(texto)
        validar(dados)
        theme = gerar_theme(dados)
        estados = [
            (SAIDA_THEME, comparar_ou_gravar(os.path.join(raiz, SAIDA_THEME), theme, verificar)),
            (SAIDA_TOKENS, comparar_ou_gravar(os.path.join(raiz, SAIDA_TOKENS), bruto, verificar)),
        ]
    except (OSError, ValueError, KeyError) as erro:
        print("gerar_theme: ERRO %s" % erro)
        return 1
    modo = "verificar" if verificar else "gravar"
    print("gerar_theme: secao=3.4 cores=%d fontes=%d escala=%d raio=%d espaco=%d modo=%s" % (
        len(dados["claro"]), len(dados["fontes"]), len(dados["escala"]),
        len(dados["raio"]), len(dados["espaco"]), modo))
    for caminho, estado in estados:
        print("arquivos: %s %s" % (caminho, estado))
    if verificar and any(estado == "DIFERE" for _, estado in estados):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
