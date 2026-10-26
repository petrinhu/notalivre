# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Leitor de XLSX só com a biblioteca padrão (zipfile e xml.etree, L-51). Lê toda célula como texto ou None.
# Números chegam como o texto gravado no XML (por exemplo "10101"), nunca convertidos: quem converte é o conversor.
#
# Guardas de DA-1.12, aplicadas ANTES de qualquer parse de XML vindo do ZIP:
#   - DOCTYPE e ENTITY são recusados pelo PRÓPRIO parser expat (xml.parsers.expat), antes do parse com
#     xml.etree. Regex sobre bytes não serve: em UTF-16 "<!ENTITY" não aparece como sequência contígua (revisão 2);
#   - cada arquivo descompactado tem teto de LIMITE_BYTES_ARQUIVO (16 MiB; o maior dos XLSX reais tem 1,1 MB);
#   - o ZIP tem teto de MAX_ARQUIVOS_ZIP entradas (256; os XLSX reais têm 21 e 27).
import io
import posixpath
import xml.etree.ElementTree as ET
import zipfile
from xml.parsers import expat

NS = "{http://schemas.openxmlformats.org/spreadsheetml/2006/main}"
NS_REL = "{http://schemas.openxmlformats.org/officeDocument/2006/relationships}"

LIMITE_BYTES_ARQUIVO = 16 * 1024 * 1024
MAX_ARQUIVOS_ZIP = 256

class ErroXlsx(ValueError):
    """XLSX recusado antes do parse (estrutura ou tamanho fora dos tetos, ou declaração DTD proibida)."""


def _coluna(referencia):
    """'AB12' -> 'AB'."""
    return "".join(caractere for caractere in referencia if caractere.isalpha())


def _conferir_contagem(pacote):
    if len(pacote.infolist()) > MAX_ARQUIVOS_ZIP:
        raise ErroXlsx(f"limite de arquivos no ZIP excedido: {len(pacote.infolist())} > {MAX_ARQUIVOS_ZIP}")


def _parte(pacote, nome):
    """Bytes de uma parte XML do ZIP, com teto de tamanho e recusa de DOCTYPE e ENTITY antes do parse."""
    tamanho_declarado = pacote.getinfo(nome).file_size
    if tamanho_declarado > LIMITE_BYTES_ARQUIVO:
        raise ErroXlsx(f"{nome}: limite de bytes descompactados excedido ({tamanho_declarado})")
    with pacote.open(nome) as arquivo:
        conteudo = arquivo.read(LIMITE_BYTES_ARQUIVO + 1)
    if len(conteudo) > LIMITE_BYTES_ARQUIVO:
        raise ErroXlsx(f"{nome}: limite de bytes descompactados excedido (lido mais de {LIMITE_BYTES_ARQUIVO})")
    _recusar_dtd(conteudo, nome)
    return conteudo


def _recusar_dtd(conteudo, nome):
    """Primeira passada, só com expat: qualquer DOCTYPE ou ENTITY levanta ErroXlsx antes de haver árvore.

    O expat decodifica a codificação real do documento (UTF-8, UTF-8 com BOM, UTF-16 com ou sem BOM), por isso
    o que ele recusa é o que o xml.etree leria. Na segunda passada (ET.fromstring), o documento já não tem DTD.
    """
    analisador = expat.ParserCreate()

    def doctype(*_argumentos):
        raise ErroXlsx(f"{nome}: DOCTYPE recusado")

    def entidade(*_argumentos):
        raise ErroXlsx(f"{nome}: ENTITY recusado")

    analisador.StartDoctypeDeclHandler = doctype
    analisador.EntityDeclHandler = entidade
    try:
        analisador.Parse(conteudo, True)
    except expat.ExpatError as erro:
        raise ErroXlsx(f"{nome}: XML malformado: {erro}") from erro


def _textos_compartilhados(raiz):
    return ["".join(t.text or "" for t in item.iter(NS + "t")) for item in raiz.iter(NS + "si")]


def _valor(celula, compartilhados):
    tipo = celula.get("t")
    if tipo == "inlineStr":
        return "".join(t.text or "" for t in celula.iter(NS + "t"))
    elemento = celula.find(NS + "v")
    if elemento is None or elemento.text is None:
        return None
    if tipo == "s":
        return compartilhados[int(elemento.text)]
    return elemento.text


def _caminho_da_aba(pacote, nome_aba):
    livro = ET.fromstring(_parte(pacote, "xl/workbook.xml"))
    relacoes = ET.fromstring(_parte(pacote, "xl/_rels/workbook.xml.rels"))
    alvo_por_id = {rel.get("Id"): rel.get("Target") for rel in relacoes}
    for aba in livro.iter(NS + "sheet"):
        if aba.get("name") == nome_aba:
            alvo = alvo_por_id[aba.get(NS_REL + "id")]
            if alvo.startswith("/"):
                return alvo.lstrip("/")
            return posixpath.normpath(posixpath.join("xl", alvo))
    raise KeyError(f"aba não encontrada: {nome_aba!r}")


def ler_aba(conteudo_xlsx, nome_aba):
    """Lista de (numero_da_linha, {coluna: texto ou None}) de toda a aba, cabeçalho incluído.

    Recebe os BYTES já conferidos por SHA-256 (fontes.py), nunca o caminho: o que foi verificado é o que se lê.
    Levanta ErroXlsx quando o pacote foge dos tetos, traz DOCTYPE ou ENTITY, ou é XML malformado.
    """
    with zipfile.ZipFile(io.BytesIO(conteudo_xlsx)) as pacote:
        _conferir_contagem(pacote)
        compartilhados = []
        if "xl/sharedStrings.xml" in pacote.namelist():
            compartilhados = _textos_compartilhados(ET.fromstring(_parte(pacote, "xl/sharedStrings.xml")))
        raiz = ET.fromstring(_parte(pacote, _caminho_da_aba(pacote, nome_aba)))
    linhas = []
    for linha in raiz.iter(NS + "row"):
        celulas = {}
        for celula in linha.findall(NS + "c"):
            celulas[_coluna(celula.get("r"))] = _valor(celula, compartilhados)
        linhas.append((int(linha.get("r")), celulas))
    return linhas
