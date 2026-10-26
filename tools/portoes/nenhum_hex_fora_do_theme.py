#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão nenhumHexForaDoTheme (NL-1.8, L-36): nenhum hexadecimal de cor fora de apps/gui/qml/Theme.qml.

Escopo: todo arquivo .qml, .js, .cpp, .h e .hpp em apps/gui, exceto o caminho exato apps/gui/qml/Theme.qml
(o caminho é comparado inteiro, não pelo nome). Se Theme.qml não existir, o portão reprova: sem o arquivo de
tokens não há o que comparar (fail-closed).

Saída obrigatória, mesmo com zero: "encontrados=N analisados=M violacoes=K".
Sai com 1 se houver violação, se algum arquivo do escopo não puder ser lido, se a varredura achar zero
arquivos (varredura quebrada, não código limpo) ou se Theme.qml faltar.

Uso: python3 -I nenhum_hex_fora_do_theme.py RAIZ_DO_REPOSITORIO
"""

import os
import re
import sys

PASTA_ESCOPO = os.path.join("apps", "gui")
ARQUIVO_THEME = os.path.join("apps", "gui", "qml", "Theme.qml")
EXTENSOES = (".qml", ".js", ".cpp", ".h", ".hpp")
RE_HEX = re.compile(r"#(?:[0-9A-Fa-f]{8}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})\b")


def coletar(raiz):
    """Devolve a lista de caminhos absolutos do escopo, sem Theme.qml, em ordem estável."""
    base = os.path.join(raiz, PASTA_ESCOPO)
    theme = os.path.normpath(os.path.join(raiz, ARQUIVO_THEME))
    achados = []
    for atual, _dirs, arquivos in os.walk(base):
        for nome in arquivos:
            if not nome.endswith(EXTENSOES):
                continue
            caminho = os.path.normpath(os.path.join(atual, nome))
            if caminho == theme:
                continue
            achados.append(caminho)
    return sorted(achados), theme


def main(argv):
    if len(argv) != 2:
        print("uso: nenhum_hex_fora_do_theme.py RAIZ_DO_REPOSITORIO", file=sys.stderr)
        return 2
    raiz = os.path.abspath(argv[1])
    encontrados, theme = coletar(raiz)
    analisados = 0
    violacoes = 0
    erros_leitura = 0
    for caminho in encontrados:
        try:
            with open(caminho, encoding="utf-8") as arq:
                linhas = arq.read().splitlines()
        except (OSError, UnicodeDecodeError) as erro:
            erros_leitura += 1
            print("ilegivel: %s (%s)" % (os.path.relpath(caminho, raiz), erro))
            continue
        analisados += 1
        for numero, linha in enumerate(linhas, start=1):
            for achado in RE_HEX.finditer(linha):
                violacoes += 1
                print("violacao: %s:%d hex=%s" % (os.path.relpath(caminho, raiz), numero, achado.group(0)))

    if not os.path.isfile(theme):
        print("theme ausente: %s" % os.path.relpath(theme, raiz))
    print("encontrados=%d analisados=%d violacoes=%d" % (len(encontrados), analisados, violacoes))

    if not os.path.isfile(theme):
        return 1
    if not encontrados or erros_leitura or analisados != len(encontrados):
        return 1
    return 1 if violacoes else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
