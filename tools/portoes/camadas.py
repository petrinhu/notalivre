#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão A2 de camadas (NL-0.3): varre as diretivas de include de src/ e apps/ contra a matriz 1.1.

Uso: python3 -I camadas.py [RAIZ]
  RAIZ: raiz da árvore (padrão: raiz do repositório, três níveis acima deste arquivo).

Uma única invocação percorre todos os arquivos, item a item, sem processo por arquivo (L-11, item 6).
Imprime sempre `encontrados=N analisados=M violacoes=K`. Sai com 1 se houver violação, se nenhum
arquivo for encontrado (varredura vazia, L-36), se a varredura tiver erro (diretório ilegível) ou se
algum arquivo encontrado não foi analisado.

Antes de procurar diretivas, o texto passa por: emendas de linha (barra invertida no fim da linha),
remoção de comentários de bloco e de linha (respeitando strings, chars e raw strings). Assim um
comentário não esconde um include e uma string com `/*` não abre comentário.

Diretivas lidas: include, include_next e import, com `#` ou digrafo `%:`, com espaços livres.
Só o literal <...> ou "..." é resolvido. Qualquer outra forma (macro, identificador, linha vazia)
é DESCONHECIDA e reprova (fail-closed).

Verificações sobre cada literal:
  1. dependência interna: origem (pelo caminho) x destino (pelo prefixo nl/ ou pelo caminho resolvido);
  2. denylist de terceiros proibidos por camada (coluna Proibido da matriz 1.1), em forma de módulo
     (<QtSql/...>) e de classe (<QSqlDatabase>);
  3. include relativo: resolvido pelo arquivo ou pelo -I da camada; se não resolve e sobe com `..`,
     é DESCONHECIDO e reprova.
A poda é só o que está FORA de src/ e apps/: a varredura nunca entra em worktrees/, build-*/ ou out/
da raiz (L-55). Nenhum diretório é podado por nome dentro de src/ ou apps/.
"""
import os
import re
import sys

EXTENSOES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl", ".ipp", ".tpp"})
RAIZES_VARRIDAS = ("src", "apps")
EMENDA = re.compile(rb"\\[ \t]*\r?\n")
DIRETIVA = re.compile(rb"^[ \t\f\v]*(?:#|%:)[ \t\f\v]*(include_next|include|import)\b([^\n]*)", re.MULTILINE)
LITERAL = re.compile(rb"[ \t\f\v]*(?:<([^>\n]*)>|\"([^\"\n]*)\")[ \t\f\v\r]*")
DESCONHECIDA = "desconhecida"

# Unidades da matriz 1.1. Origem "mid" cobre mid:composicao (ver unidade_origem).
PERMITIDOS = {
    "front:cli": frozenset({"front:cli", "foundation:base", "mid:composicao"}),
    "front:gui": frozenset({"front:gui", "foundation:base", "mid:composicao"}),
    "mid": frozenset({"mid", "mid:composicao", "back", "foundation", "foundation:base"}),
    "back": frozenset({"back", "foundation:base"}),
    "foundation": frozenset({"foundation", "foundation:base"}),
    "foundation:base": frozenset({"foundation:base"}),
}

# Diretórios -I de cada camada, para resolver include relativo (ver resolver_relativo).
DIRS_INCLUDE = {
    "back": ("src/back/include",),
    "mid": ("src/mid/include",),
    "foundation:base": ("src/foundation/base/include",),
    "foundation": ("src/foundation/include",),
}

# Denylist: terceiros proibidos pela coluna Proibido da matriz 1.1. Casa com o texto do include.
# Cada padrão cobre a forma de módulo (QtSql/QSqlDatabase) e a de classe (QSqlDatabase).
_SQL = [r"^QtSql(/|$)", r"(^|/)QSql[A-Za-z]*$", r"^sqlite3\.h$", r"^mysql", r"^libpq", r"^postgres"]
_REDE = [
    r"^QtNetwork(/|$)", r"(^|/)QNetwork[A-Za-z]*$", r"(^|/)QTcpSocket$", r"(^|/)QTcpServer$",
    r"(^|/)QUdpSocket$", r"(^|/)QWebSocket$", r"^QtHttpServer(/|$)", r"(^|/)QHttp[A-Za-z]*$",
    r"^curl(/|$)", r"^asio(/|$)", r"^boost/asio", r"^sys/socket\.h$", r"^netinet/", r"^arpa/inet\.h$",
    r"^netdb\.h$",
]
_CRIPTO = [
    r"^openssl(/|$)", r"^sodium(/|$)", r"^sodium\.h$", r"^gcrypt", r"^botan",
    r"(^|/)QCryptographicHash$", r"(^|/)QSsl[A-Za-z]*$",
]
_XML_BIBLIOTECA = [r"^libxml(2)?(/|$)", r"^xmlsec(1)?(/|$)"]
_XML_QT = [r"^QtXml(/|$)", r"(^|/)QDom[A-Za-z]*$", r"(^|/)QXml[A-Za-z]*$"]
_ARQUIVO = [
    r"^fstream$", r"^filesystem$", r"^experimental/filesystem$", r"(^|/)QFile[A-Za-z]*$",
    r"(^|/)QDir[A-Za-z]*$", r"(^|/)QSaveFile$", r"(^|/)QTemporaryFile$", r"(^|/)QTemporaryDir$",
]
_GUI_QT = [
    r"^QtGui(/|$)", r"^QtQuick", r"^QtQml(/|$)", r"^QtWidgets(/|$)", r"(^|/)QQuick[A-Za-z]*$",
    r"(^|/)QQml[A-Za-z]*$", r"(^|/)QWidget$", r"(^|/)QApplication$", r"(^|/)QImage$", r"(^|/)QPixmap$",
    r"(^|/)QPainter$", r"(^|/)QWindow$", r"(^|/)QGuiApplication$", r"(^|/)QFont$", r"(^|/)QColor$",
]
_QML_WIDGETS = [
    r"^QtQuick", r"^QtQml(/|$)", r"^QtWidgets(/|$)", r"(^|/)QQuick[A-Za-z]*$", r"(^|/)QQml[A-Za-z]*$",
    r"(^|/)QWidget$", r"(^|/)QApplication$",
]
DENYLIST = {
    "back": _REDE + _SQL + _CRIPTO + _XML_QT + _XML_BIBLIOTECA + _ARQUIVO + _GUI_QT,
    "mid": _QML_WIDGETS,
    "front": _SQL + _REDE + _CRIPTO + _XML_BIBLIOTECA,
}


def arquivos_fonte(raiz, erros):
    """Arquivos C/C++ de src/ e apps/, ordenados. Erro de diretório vai para `erros` (INCOMPLETA)."""
    def registrar(erro):
        caminho = getattr(erro, "filename", None) or "?"
        motivo = getattr(erro, "strerror", None) or str(erro)
        erros.append("ERRO VARREDURA INCOMPLETA: {}: {}".format(caminho, motivo))

    achados = []
    for base in RAIZES_VARRIDAS:
        for dirpath, _dirnames, filenames in os.walk(os.path.join(raiz, base), onerror=registrar):
            for nome in filenames:
                if os.path.splitext(nome)[1].lower() in EXTENSOES:
                    achados.append(os.path.join(dirpath, nome))
    return sorted(achados)


def relativo(raiz, caminho):
    return os.path.relpath(caminho, raiz).replace(os.sep, "/")


def unidade_do_caminho(rel):
    """Unidade da matriz de um arquivo, pelo caminho relativo à raiz. None = fora da matriz."""
    partes = rel.split("/")
    if partes[0] == "apps" and len(partes) >= 3 and partes[1] in ("cli", "gui"):
        return "front:" + partes[1]
    if partes[0] == "src" and len(partes) >= 3:
        if partes[1] == "foundation":
            return "foundation:base" if partes[2] == "base" else "foundation"
        if partes[1] == "back":
            return "back"
        if partes[1] == "mid":
            return "mid:composicao" if partes[2] == "composicao" else "mid"
    return None


def unidade_origem(unidade):
    """Origem usa a regra da camada: mid:composicao age como mid."""
    return "mid" if unidade.startswith("mid") else unidade


def base_denylist(origem):
    """Chave da denylist da camada de origem. Foundation não tem denylist: é ela que embrulha terceiros."""
    if origem.startswith("front"):
        return "front"
    if origem.startswith("mid"):
        return "mid"
    if origem == "back":
        return "back"
    return None


def unidade_do_prefixo(alvo):
    """Destino de um include nl/... pelo prefixo. Prefixo nl/ desconhecido vira DESCONHECIDA (fail-closed)."""
    prefixos = (
        ("nl/base/", "foundation:base"),
        ("nl/foundation/", "foundation"),
        ("nl/back/", "back"),
        ("nl/mid/composicao/", "mid:composicao"),
        ("nl/mid/", "mid"),
    )
    for prefixo, unidade in prefixos:
        if alvo.startswith(prefixo):
            return unidade
    return DESCONHECIDA


def resolver_relativo(raiz, caminho, origem, alvo):
    """Include sem prefixo nl/: resolve pelo arquivo ou pelo -I da camada. None = externo (não conta)."""
    if alvo.startswith("/"):
        return DESCONHECIDA
    bases = [os.path.dirname(caminho)]
    bases += [os.path.join(raiz, d) for d in DIRS_INCLUDE.get(unidade_origem(origem), ())]
    for base in bases:
        candidato = os.path.normpath(os.path.join(base, alvo))
        if os.path.isfile(candidato):
            return unidade_do_caminho(relativo(raiz, candidato)) or DESCONHECIDA
    if ".." in alvo.split("/"):
        return DESCONHECIDA
    return None


def destino_do_include(raiz, caminho, origem, alvo):
    if alvo.startswith("nl/"):
        return unidade_do_prefixo(alvo)
    return resolver_relativo(raiz, caminho, origem, alvo)


def terceiro_proibido(origem, alvo):
    base = base_denylist(origem)
    if base is None:
        return False
    return any(re.search(padrao, alvo) for padrao in DENYLIST[base])


def permitido(origem, destino):
    return destino in PERMITIDOS.get(unidade_origem(origem), frozenset())


def remover_emendas(dados):
    """Fase 2 do C++: apaga barra invertida + quebra de linha. Devolve o texto e a posição original de cada byte."""
    apagar = bytearray(len(dados))
    for casamento in EMENDA.finditer(dados):
        for k in range(casamento.start(), casamento.end()):
            apagar[k] = 1
    texto = bytearray()
    posicoes = []
    for k, byte in enumerate(dados):
        if not apagar[k]:
            texto.append(byte)
            posicoes.append(k)
    return bytes(texto), posicoes


def _fim_literal(texto, inicio):
    """Índice logo após uma string ou char literal que começa em `inicio`. Quebra de linha encerra."""
    quote = texto[inicio]
    j = inicio + 1
    while j < len(texto):
        if texto[j] == 0x5C:  # barra invertida: pula o caractere escapado
            j += 2
            continue
        if texto[j] == quote:
            return j + 1
        if texto[j] == 0x0A:
            return j
        j += 1
    return len(texto)


def _fim_raw(texto, aspas):
    """Índice após uma raw string R"delim(...)delim" cujas aspas estão em `aspas`, ou None se malformada."""
    paren = texto.find(b"(", aspas + 1, aspas + 18)
    if paren < 0 or b"\n" in texto[aspas + 1:paren]:
        return None
    marca = b")" + texto[aspas + 1:paren] + b'"'
    fim = texto.find(marca, paren + 1)
    return len(texto) if fim < 0 else fim + len(marca)


def _apagar(saida, inicio, fim):
    for k in range(inicio, fim):
        if saida[k] != 0x0A:
            saida[k] = 0x20


def remover_comentarios(texto):
    """Fase 3 do C++: troca comentário por espaço, mantendo quebras de linha e o tamanho do texto."""
    saida = bytearray(texto)
    i = 0
    while i < len(texto):
        if texto[i] == 0x22 and i > 0 and texto[i - 1] == 0x52:  # R"..." raw string
            fim = _fim_raw(texto, i)
            if fim is not None:
                i = fim
                continue
        if texto[i] in (0x22, 0x27):  # " ou '
            i = _fim_literal(texto, i)
            continue
        if texto[i:i + 2] == b"//":
            fim = texto.find(b"\n", i)
            fim = len(texto) if fim < 0 else fim
            _apagar(saida, i, fim)
            i = fim
            continue
        if texto[i:i + 2] == b"/*":
            fim = texto.find(b"*/", i + 2)
            fim = len(texto) if fim < 0 else fim + 2
            _apagar(saida, i, fim)
            i = fim
            continue
        i += 1
    return bytes(saida)


def limpar_fonte(dados):
    """Texto pronto para ler diretivas, e a posição original de cada byte (para o número da linha)."""
    texto, posicoes = remover_emendas(dados)
    return remover_comentarios(texto), posicoes


def verificar_literal(raiz, caminho, origem, rel, linha, alvo):
    """Violação de um include literal, ou None. None também quando é externo e não é terceiro proibido."""
    destino = destino_do_include(raiz, caminho, origem, alvo)
    if destino is None:
        if terceiro_proibido(origem, alvo):
            return "VIOLACAO {}:{}: {} inclui {} (terceiro proibido na camada {})".format(
                rel, linha, origem, alvo, base_denylist(origem))
        return None
    if destino == DESCONHECIDA:
        return "VIOLACAO {}:{}: include {} fora da matriz 1.1 (prefixo ou caminho nao resolvido, fail-closed)".format(
            rel, linha, alvo)
    if not permitido(origem, destino):
        return "VIOLACAO {}:{}: {} inclui {} ({}) proibido pela matriz 1.1".format(
            rel, linha, origem, alvo, destino)
    return None


def verificar_arquivo(raiz, caminho, dados):
    """Devolve as violações de um arquivo já lido. Cada uma traz arquivo:linha."""
    rel = relativo(raiz, caminho)
    origem = unidade_do_caminho(rel)
    if origem is None:
        return ["VIOLACAO {}:1: arquivo fora da matriz 1.1 (diretorio nao mapeado)".format(rel)]
    texto, posicoes = limpar_fonte(dados)
    falhas = []
    for casamento in DIRETIVA.finditer(texto):
        linha = dados.count(b"\n", 0, posicoes[casamento.start()]) + 1
        literal = LITERAL.fullmatch(casamento.group(2))
        if literal is None:
            resto = casamento.group(2).decode("utf-8", "replace").strip()
            falhas.append("VIOLACAO {}:{}: #{} nao literal ({}), forma nao padrao, fail-closed".format(
                rel, linha, casamento.group(1).decode("ascii"), resto or "vazio"))
            continue
        bruto = literal.group(1) if literal.group(1) is not None else literal.group(2)
        alvo = bruto.decode("utf-8", "replace").strip()
        if not alvo:
            falhas.append("VIOLACAO {}:{}: include vazio, fail-closed".format(rel, linha))
            continue
        falha = verificar_literal(raiz, caminho, origem, rel, linha, alvo)
        if falha is not None:
            falhas.append(falha)
    return falhas


def varrer(raiz):
    """Varre tudo numa passada. Devolve (encontrados, analisados, violações, erros de varredura)."""
    erros = []
    arquivos = arquivos_fonte(raiz, erros)
    analisados = 0
    violacoes = []
    for caminho in arquivos:
        try:
            with open(caminho, "rb") as arquivo:
                dados = arquivo.read()
        except OSError as erro:
            erros.append("ERRO LEITURA {}: {}".format(relativo(raiz, caminho), erro))
            continue
        analisados += 1
        violacoes.extend(verificar_arquivo(raiz, caminho, dados))
    return len(arquivos), analisados, violacoes, erros


def raiz_padrao():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main(argv):
    if len(argv) > 2:
        print("uso: camadas.py [RAIZ]", file=sys.stderr)
        return 2
    raiz = os.path.abspath(argv[1]) if len(argv) == 2 else raiz_padrao()
    encontrados, analisados, violacoes, erros = varrer(raiz)
    for linha in violacoes + erros:
        print(linha)
    if encontrados == 0:
        print("VARREDURA VAZIA: nenhum arquivo C/C++ em src/ nem apps/")
    if analisados != encontrados:
        print("VARREDURA INCOMPLETA: analisados diferente de encontrados")
    print("encontrados={} analisados={} violacoes={}".format(encontrados, analisados, len(violacoes)))
    falhou = bool(violacoes) or bool(erros) or encontrados == 0 or analisados != encontrados
    return 1 if falhou else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
