#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão de distro aposentada e digest ancorado (NL-3.0g, decisão do líder de 05/11/2026).

Uso: python3 -I distro_base.py RAIZ          verifica os arquivos rastreados por git de RAIZ
     python3 -I distro_base.py --autoteste   controles em memória: positivo e uma sabotagem por regra

Listas, lidas de RAIZ/tools/portoes:
  distro_aposentada.txt            padrão regex (linha 1); linhas vazias e as que começam com '#' são ignoradas
  distro_aposentada_excecoes.txt   formato `caminho contagem`; contagem = ocorrências do padrão no conteúdo
  distro_digest_aprovado.txt       um digest (64 hex) por linha, o aprovado para a imagem da matriz

Regras (rc 1 se alguma falhar):
  1. arquivo que casa o padrão, no caminho ou no conteúdo, e não está nas exceções;
  2. exceção com contagem diferente da medida, para mais ou para menos, ou caminho que não está no git;
  3. digest da imagem (depois do prefixo de imagem e do arroba) com menos de MIN_OCORRENCIAS_DIGEST ocorrências,
     com mais de um valor distinto, ou com valor fora de distro_digest_aprovado.txt (lista vazia reprova);
  4. varredura vazia (zero arquivos rastreados).
Atualizar o digest da imagem é ato do diff: muda o arquivo de aprovados, e a revisão confere.
Um único `git ls-files` e leitura em processo: nenhum processo por arquivo (L-11).
"""

import os
import re
import subprocess
import sys

MIN_OCORRENCIAS_DIGEST = 4
RE_DIGEST = re.compile(r"fedora@sha256:([0-9a-f]{64})")
RE_HEX64 = re.compile(r"^[0-9a-f]{64}$")
# Montado por concatenação: a string completa não pode aparecer neste arquivo, senão o portão reprova a si mesmo.
PADRAO_CONTROLE = "(?i)fedora[ _:-]?" + "44|fc" + "44"


def carregar_padroes(texto):
    padroes = []
    for linha in texto.splitlines():
        limpa = linha.strip()
        if limpa and not limpa.startswith("#"):
            padroes.append(re.compile(limpa))
    return padroes


def carregar_excecoes(texto):
    excecoes = {}
    for numero, linha in enumerate(texto.splitlines(), 1):
        limpa = linha.strip()
        if not limpa or limpa.startswith("#"):
            continue
        partes = limpa.split()
        if len(partes) != 2 or not partes[1].isdigit():
            raise ValueError(f"exceções, linha {numero}: fora do formato 'caminho contagem'")
        excecoes[partes[0]] = int(partes[1])
    return excecoes


def carregar_digests_aprovados(texto):
    aprovados = set()
    for linha in texto.splitlines():
        limpa = linha.strip()
        if not limpa or limpa.startswith("#"):
            continue
        if not RE_HEX64.match(limpa):
            raise ValueError(f"digests aprovados: linha fora do formato (64 hex): {limpa[:20]}")
        aprovados.add(limpa)
    return aprovados


def contar_ocorrencias(padroes, texto):
    return sum(len(padrao.findall(texto)) for padrao in padroes)


def verificar_distro(arquivos, padroes, excecoes):
    """Regras 1, 2 e 4 sobre {caminho: conteúdo}. Devolve (aposentadas, violacoes)."""
    violacoes = []
    aposentadas = 0
    for caminho, texto in arquivos.items():
        ocorrencias = contar_ocorrencias(padroes, texto)
        casa_nome = any(padrao.search(caminho) for padrao in padroes)
        if caminho in excecoes:
            if excecoes[caminho] != ocorrencias:
                violacoes.append((caminho, f"exceção com contagem {excecoes[caminho]}, medida {ocorrencias}"))
        elif ocorrencias > 0 or casa_nome:
            aposentadas += 1
            violacoes.append((caminho, "distro aposentada fora das exceções"))
    for caminho in excecoes:
        if caminho not in arquivos:
            violacoes.append((caminho, "exceção sem arquivo rastreado"))
    return aposentadas, violacoes


def verificar_digests(arquivos, aprovados):
    """Regra 3 sobre `fedora@sha256:`. Devolve (digests distintos, violacoes)."""
    digests = {}
    total = 0
    for caminho, texto in arquivos.items():
        for casa in RE_DIGEST.finditer(texto):
            total += 1
            digests.setdefault(casa.group(1), []).append(caminho)
    violacoes = []
    if len(digests) > 1:
        violacoes.append(("-", f"digest distinto: {len(digests)} valores em fedora@sha256:"))
    if total < MIN_OCORRENCIAS_DIGEST:
        violacoes.append(("-", f"fedora@sha256: com {total} ocorrências (mínimo {MIN_OCORRENCIAS_DIGEST})"))
    if not aprovados:
        violacoes.append(("-", "lista de digests aprovados vazia (fail-closed, distro_digest_aprovado.txt)"))
    for digest in sorted(digests):
        if digest not in aprovados:
            violacoes.append(("-", f"digest fora da lista aprovada (distro_digest_aprovado.txt): {digest[:12]}... "
                                   f"em {len(digests[digest])} arquivo(s)"))
    return len(digests), violacoes


def analisar(arquivos, texto_aposentada, texto_excecoes, texto_aprovados):
    """Aplica as 4 regras. Devolve dict com as contagens da linha de saída e a lista de violações."""
    violacoes = []
    if not arquivos:
        violacoes.append(("-", "varredura vazia: nenhum arquivo rastreado"))
    padroes = carregar_padroes(texto_aposentada)
    if not padroes:
        violacoes.append(("-", "lista de padrões vazia"))
    try:
        excecoes = carregar_excecoes(texto_excecoes)
    except ValueError as erro:
        violacoes.append(("-", str(erro)))
        excecoes = {}
    try:
        aprovados = carregar_digests_aprovados(texto_aprovados)
    except ValueError as erro:
        violacoes.append(("-", str(erro)))
        aprovados = set()
    aposentadas, violacoes_distro = verificar_distro(arquivos, padroes, excecoes)
    digests, violacoes_digest = verificar_digests(arquivos, aprovados)
    violacoes.extend(violacoes_distro)
    violacoes.extend(violacoes_digest)
    return {
        "arquivos": len(arquivos),
        "aposentadas": aposentadas,
        "excecoes": len(excecoes),
        "digests": digests,
        "violacoes": violacoes,
    }


def ler_arquivos_rastreados(raiz):
    saida = subprocess.run(["git", "-C", raiz, "ls-files", "-z"], capture_output=True, check=False)
    if saida.returncode != 0:
        raise RuntimeError("git ls-files falhou: " + saida.stderr.decode("utf-8", "replace").strip())
    arquivos = {}
    for nome in saida.stdout.decode("utf-8", "surrogateescape").split("\0"):
        if not nome:
            continue
        with open(os.path.join(raiz, nome), "rb") as fonte:
            arquivos[nome] = fonte.read().decode("utf-8", "replace")
    return arquivos


def ler_lista(raiz, nome):
    with open(os.path.join(raiz, "tools", "portoes", nome), "r", encoding="utf-8") as lista:
        return lista.read()


def verificar(raiz):
    try:
        arquivos = ler_arquivos_rastreados(raiz)
        texto_aposentada = ler_lista(raiz, "distro_aposentada.txt")
        texto_excecoes = ler_lista(raiz, "distro_aposentada_excecoes.txt")
        texto_aprovados = ler_lista(raiz, "distro_digest_aprovado.txt")
    except (OSError, RuntimeError) as erro:
        print(f"REPROVA: {erro}")
        return 1
    resultado = analisar(arquivos, texto_aposentada, texto_excecoes, texto_aprovados)
    for caminho, mensagem in resultado["violacoes"]:
        print(f"VIOLACAO {caminho}: {mensagem}")
    print(
        f"arquivos={resultado['arquivos']} aposentadas={resultado['aposentadas']} "
        f"excecoes={resultado['excecoes']} digests={resultado['digests']} "
        f"violacoes={len(resultado['violacoes'])}"
    )
    if resultado["violacoes"]:
        print("REPROVA: distro aposentada ou digest fora da política (NL-3.0g)")
        return 1
    print("PASSA")
    return 0


DIGEST_APROVADO_CONTROLE = "a" * 64


def casos_do_autoteste():
    digest = DIGEST_APROVADO_CONTROLE
    outro = "b" * 64
    bons = {f"ci/{i}.yml": f"image: fedora@sha256:{digest}\n" for i in range(4)}
    return [
        ("positivo", dict(bons), "", 0),
        ("distro-citada-em-tests", {**bons, "tests/x.txt": "alvo " + "fedora" + "44"}, "", 1),
        ("distro-no-nome-do-arquivo", {**bons, "tools/build-" + "fedora" + "44.sh": ""}, "", 1),
        ("excecao-contagem-errada", {**bons, "docs/a.md": "x " + "fedora" + "44"}, "docs/a.md 2\n", 1),
        ("excecao-correta", {**bons, "docs/a.md": "x " + "fedora" + "44"}, "docs/a.md 1\n", 0),
        ("excecao-sem-arquivo", dict(bons), "docs/fantasma.md 0\n", 1),
        ("digest-duplo", {**bons, "x.yml": f"fedora@sha256:{outro}"}, "", 1),
        ("digest-insuficiente", {k: v for k, v in list(bons.items())[:3]}, "", 1),
        ("digest-nao-aprovado", {k: v.replace(digest, outro) for k, v in bons.items()}, "", 1),
        ("digest-de-distro-antiga", {k: v.replace(digest, "43b29f65a41eb9c35e1cd5323e3bdf3b655c2357a9f4f1ff2f9c2798e5045d80")
                                     for k, v in bons.items()}, "", 1),
        ("varredura-vazia", {}, "", 1),
        ("excecao-ilegivel", dict(bons), "docs/a.md dois\n", 1),
    ]


def autoteste():
    falhas = 0
    texto_aposentada = PADRAO_CONTROLE + "\n"
    texto_aprovados = "# digests aprovados de controle\n" + DIGEST_APROVADO_CONTROLE + "\n"
    for nome, arquivos, excecoes, esperado in casos_do_autoteste():
        resultado = analisar(arquivos, texto_aposentada, excecoes, texto_aprovados)
        rc = 1 if resultado["violacoes"] else 0
        ok = rc == esperado
        falhas += 0 if ok else 1
        print(f"[{'VERDE' if ok else 'VERMELHO'}] {nome}: rc={rc} (esperado {esperado})")
    print(f"AUTOTESTE DISTRO_BASE: falhas={falhas}")
    return 0 if falhas == 0 else 1


def main(argv):
    if argv == ["--autoteste"]:
        return autoteste()
    if len(argv) != 1:
        print("uso: distro_base.py RAIZ | --autoteste")
        return 1
    return verificar(argv[0])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
