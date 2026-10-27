# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da NL-1.10 (tools/tabelas). Stdlib, python3 -I. Cada teste entra no CTest como um processo próprio:
#   python3 -I test_tabelas.py <nome-do-teste>
# Sem rede e sem os XLSX reais (L-36: nada de teste que pula se faltar arquivo). Os cenários de XLSX são
# fabricados dentro do próprio teste, em memória ou em diretório temporário.
import contextlib
import copy
import hashlib
import io
import json
import os
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock
from xml.sax.saxutils import escape

# python3 -I não põe o diretório do script no sys.path: o caminho de tools/tabelas é explícito.
RAIZ = Path(__file__).resolve().parents[3]
if not (RAIZ / "tools" / "tabelas" / "normaliza.py").is_file():
    sys.exit("FALHA: árvore sem tools/tabelas. Se rodou pelo contêiner, o copiar_fonte de "
             "tools/conteiner/build_local.sh não copia tools/tabelas (lacuna registrada na NL-1.10).")
sys.path.insert(0, str(RAIZ / "tools" / "tabelas"))

import converter  # noqa: E402
import conversores  # noqa: E402
import normaliza  # noqa: E402
import xlsx_leitor  # noqa: E402

NS_PLANILHA = ('xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
               'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"')

# Contagens esperadas dos cenários fabricados (uma linha de dado por tabela, mais o que é de propósito).
ESPERADO_FABRICADO = {
    "ctribnac": {"linhas": 2, "com_codigo": 1, "sem_codigo": 1, "codigo_6_digitos": 0,
                 "codigo_5_digitos": 1, "grupo_99": 0},
    "nbs": {"linhas": 1, "pontuados": 1, "sem_pontos": 0, "sem_descricao": 0},
    "municipios": {"linhas": 1, "siglas_planilha": 1, "siglas_derivadas": 0},
    "paises": {"linhas": 1},
    "localidades": {"linhas": 1, "vazias_finais": 1},
}
SAIDAS_ESPERADAS = ["ctribnac.json", "nbs.json", "municipios.json", "paises.json", "localidades.json"]
CABECALHO_SERVICO_LISTA = ["CÓDIGO DE TRIBUTAÇÃO NACIONAL", "ITEM", "SUBITEM", "DESDOBRO NACIONAL", "DESCRIÇÃO"]


def _planilha(linhas):
    corpo = []
    for indice, valores in enumerate(linhas, 1):
        celulas = []
        for coluna, valor in enumerate(valores):
            if valor is None:
                continue
            referencia = f"{chr(ord('A') + coluna)}{indice}"
            celulas.append(f'<c r="{referencia}" t="inlineStr"><is><t>{escape(valor)}</t></is></c>')
        corpo.append(f'<row r="{indice}">{"".join(celulas)}</row>')
    return (f'<?xml version="1.0" encoding="UTF-8"?><worksheet {NS_PLANILHA}>'
            f'<sheetData>{"".join(corpo)}</sheetData></worksheet>').encode("utf-8")


def _xlsx(abas, sheet1=None, extras=0):
    """XLSX mínimo e válido. sheet1 substitui o XML da primeira aba; extras acrescenta arquivos ao ZIP."""
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as pacote:
        folhas, relacoes = [], []
        for indice, (nome, linhas) in enumerate(abas, 1):
            folhas.append(f'<sheet name="{escape(nome)}" sheetId="{indice}" r:id="rId{indice}"/>')
            relacoes.append(f'<Relationship Id="rId{indice}" Type="urn:worksheet" '
                            f'Target="worksheets/sheet{indice}.xml"/>')
            conteudo = sheet1 if (indice == 1 and sheet1 is not None) else _planilha(linhas)
            pacote.writestr(f"xl/worksheets/sheet{indice}.xml", conteudo)
        pacote.writestr("xl/workbook.xml", f'<?xml version="1.0" encoding="UTF-8"?>'
                                           f'<workbook {NS_PLANILHA}><sheets>{"".join(folhas)}</sheets></workbook>')
        pacote.writestr("xl/_rels/workbook.xml.rels",
                        '<?xml version="1.0" encoding="UTF-8"?>'
                        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
                        f'{"".join(relacoes)}</Relationships>')
        for indice in range(extras):
            pacote.writestr(f"docProps/extra{indice}.xml", "<x/>")
    return buffer.getvalue()


def _abas_a():
    return [
        ("TAB.MUN_IBGE", [["NOME UF", "Sigla UF", "NOME MUNICÍPIO", "CÓDIGO COMPLETO"],
                          ["Rondônia", "RO", "Alta Floresta D'Oeste", "1100015"]]),
        ("TAB.PAÍS_ISO2", [["PAÍS ISO 2", "NOME PAÍS"], ["BR", "Brasil"]]),
        ("TAB.LOC.GERAL", [["LOCALIDADE DE \nPRESTAÇÃO DE SERVIÇO\nGERAL", "CÓDIGO COMPLETO"],
                           ["ÁGUAS MARÍTIMAS", "0000000"], []]),
    ]


def _abas_b():
    return [
        ("LISTA.SERV.NAC.", [CABECALHO_SERVICO_LISTA, [None, "1", "0", "0", "Grupo"],
                             ["10101", "1", "1", "1", "Análise"]]),
        ("LISTA.NBS_v2.0", [["CÓDIGO NBS", "DESCRIÇÃO"], ["1.0101.1", "Construção"]]),
    ]


def _anexo(arquivo, conteudo, sha_errado=False):
    sha = "0" * 64 if sha_errado else hashlib.sha256(conteudo).hexdigest()
    return {"arquivo": arquivo, "url": "https://exemplo.invalido/" + arquivo, "versao": "1.00",
            "data_publicacao": "2025-12-10", "bytes": len(conteudo), "sha256": sha}


def _cenario(diretorio, bytes_a=None, sha_a_errado=False):
    """Escreve os dois XLSX e o fontes.json no diretório; devolve (caminho do fontes.json, pasta de saída)."""
    conteudo_a = bytes_a if bytes_a is not None else _xlsx(_abas_a())
    conteudo_b = _xlsx(_abas_b())
    for nome, conteudo in (("anexo_a.xlsx", conteudo_a), ("anexo_b.xlsx", conteudo_b)):
        with open(os.path.join(diretorio, nome), "wb") as arquivo:
            arquivo.write(conteudo)
    documento = {"versao_formato": 1, "data_download": "2026-10-25", "anexos": {
        "A": _anexo("anexo_a.xlsx", conteudo_a, sha_errado=sha_a_errado),
        "B": _anexo("anexo_b.xlsx", conteudo_b)}}
    caminho_fontes = os.path.join(diretorio, "fontes.json")
    with open(caminho_fontes, "w", encoding="utf-8") as arquivo:
        json.dump(documento, arquivo)
    return caminho_fontes, os.path.join(diretorio, "saida")


def _rodar(diretorio, caminho_fontes, saida, esperado):
    """Roda o conversor com ESPERADO trocado; a saída do stdout e do stderr fica fora do log do teste."""
    with mock.patch.dict(converter.ESPERADO, esperado), \
            contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        return converter.main(["--xlsx-dir", diretorio, "--saida", saida, "--fontes", caminho_fontes])


def _sem_saida(saida):
    return not os.path.isdir(saida) or not os.listdir(saida)


class TestesTabelas(unittest.TestCase):
    def nbsSemDescricaoMantidaEContada(self):
        # Código sentinela da fonte (99999) vem sem texto: fica na tabela, com descrição vazia, e é contado.
        linhas = [(1, {"A": "CÓDIGO NBS", "B": "DESCRIÇÃO"}), (2, {"A": "99999", "B": None})]
        resultado = conversores.converter_nbs(linhas)
        self.assertEqual(resultado.erros, [])
        self.assertEqual(resultado.registros, [{"codigo": "99999", "descricao": ""}])
        self.assertEqual(resultado.contagens["sem_descricao"], 1)

    def cTribNac10101ViraTexto010101(self):
        self.assertEqual(normaliza.ctribnac_texto("10101"), "010101")

    def cTribNacConfereComItemSubitemDesdobro(self):
        self.assertTrue(normaliza.ctribnac_confere("010101", item=1, subitem=1, desdobro=1))
        self.assertTrue(normaliza.ctribnac_confere("100101", item=10, subitem=1, desdobro=1))
        self.assertFalse(normaliza.ctribnac_confere("010102", item=1, subitem=1, desdobro=1))

    def municipioComSeteDigitos(self):
        self.assertEqual(normaliza.codigo_municipio("1100015"), "1100015")
        with self.assertRaises(ValueError):
            normaliza.codigo_municipio("110001")

    def nbsComFormatoPontuadoPreservado(self):
        self.assertEqual(normaliza.nbs_texto("1.0101.11.00"), "1.0101.11.00")
        self.assertEqual(normaliza.nbs_texto("1.0101.1"), "1.0101.1")
        self.assertEqual(normaliza.nbs_texto("10101"), "10101")

    def cTribNac99XXXXIdentificavel(self):
        self.assertTrue(normaliza.grupo_sem_issqn("990101"))
        self.assertFalse(normaliza.grupo_sem_issqn("010101"))
        self.assertEqual(normaliza.ctribnac_texto("990101"), "990101")

    # Guardas do conversor: cada uma vista vermelha com a guarda desligada numa cópia (NL-1.10-vermelho-guardas.log).
    def servicoItemSubitemDesdobroInconsistenteFalha(self):
        linhas = [(1, dict(zip("ABCDE", CABECALHO_SERVICO_LISTA))),
                  (2, {"A": "010101", "B": "1", "C": "1", "D": "2", "E": "Análise"})]
        resultado = conversores.converter_servico(linhas)
        self.assertEqual(len(resultado.erros), 1)
        self.assertEqual(resultado.registros, [])

    def convSHAErradoSaiComRc1SemGravar(self):
        with tempfile.TemporaryDirectory() as diretorio:
            caminho_fontes, saida = _cenario(diretorio, sha_a_errado=True)
            self.assertEqual(_rodar(diretorio, caminho_fontes, saida, ESPERADO_FABRICADO), 1)
            self.assertTrue(_sem_saida(saida))

    def contagemAlteradaSaiComRc1SemGravar(self):
        esperado = copy.deepcopy(ESPERADO_FABRICADO)
        esperado["ctribnac"]["linhas"] = 99
        with tempfile.TemporaryDirectory() as diretorio:
            caminho_fontes, saida = _cenario(diretorio)
            self.assertEqual(_rodar(diretorio, caminho_fontes, saida, esperado), 1)
            self.assertTrue(_sem_saida(saida))

    def conversorFabricadoGravaQuandoBate(self):
        # Controle positivo: com SHA certo e contagens certas, o mesmo cenário grava as cinco saídas.
        with tempfile.TemporaryDirectory() as diretorio:
            caminho_fontes, saida = _cenario(diretorio)
            self.assertEqual(_rodar(diretorio, caminho_fontes, saida, ESPERADO_FABRICADO), 0)
            for arquivo in SAIDAS_ESPERADAS:
                self.assertTrue(os.path.isfile(os.path.join(saida, arquivo)), arquivo)

    # DA-1.10: Águas Marítimas (TAB.LOC.GERAL do Anexo A) entra como tabela embutida.
    def aguasMaritimasConvertida(self):
        linhas = [(1, {"A": "LOCALIDADE DE \nPRESTAÇÃO DE SERVIÇO\nGERAL", "B": "CÓDIGO COMPLETO"}),
                  (2, {"A": "ÁGUAS MARÍTIMAS", "B": "0000000"}),
                  (3, {})]
        resultado = conversores.converter_localidades(linhas)
        self.assertEqual(resultado.erros, [])
        self.assertEqual(resultado.registros, [{"codigo": "0000000", "nome": "ÁGUAS MARÍTIMAS"}])
        self.assertEqual(resultado.contagens["vazias_finais"], 1)

    # DA-1.12: recusas do XLSX antes do parse. Cada uma com ZIP fabricado e teste próprio.
    def xlsxValidoEhLido(self):
        linhas = xlsx_leitor.ler_aba(_xlsx(_abas_b()), "LISTA.NBS_v2.0")
        self.assertEqual(linhas[1], (2, {"A": "1.0101.1", "B": "Construção"}))

    def xlsxComDoctypeRecusado(self):
        xml = (b'<?xml version="1.0"?><!DOCTYPE worksheet><worksheet '
               b'xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><sheetData/></worksheet>')
        with self.assertRaisesRegex(xlsx_leitor.ErroXlsx, "DOCTYPE recusado"):
            xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml), "LISTA.SERV.NAC.")

    def xlsxComEntityRecusado(self):
        xml = (b'<?xml version="1.0"?><!DOCTYPE worksheet [<!ENTITY x "boom">]><worksheet '
               b'xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">'
               b'<sheetData><row r="1"><c r="A1" t="inlineStr"><is><t>&x;</t></is></c></row></sheetData>'
               b'</worksheet>')
        # O expat avisa o DOCTYPE antes de qualquer ENTITY: a recusa pode vir como DOCTYPE ou ENTITY.
        with self.assertRaisesRegex(xlsx_leitor.ErroXlsx, "(DOCTYPE|ENTITY) recusado"):
            xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml), "LISTA.SERV.NAC.")

    def xlsxArquivoDescompactadoGrandeRecusado(self):
        xml = _planilha(_abas_b()[0][1]) + b" " * 2048
        with mock.patch.object(xlsx_leitor, "LIMITE_BYTES_ARQUIVO", 1024):
            with self.assertRaisesRegex(xlsx_leitor.ErroXlsx, "limite de bytes descompactados"):
                xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml), "LISTA.SERV.NAC.")

    def xlsxUtf16ComEntityRecusado(self):
        # Regressão da 2ª revisão: em UTF-16 a regex sobre bytes não acha "<!ENTITY" e o expat expandia a entidade.
        xml = ('<?xml version="1.0" encoding="UTF-16"?><!DOCTYPE worksheet [<!ENTITY x "BOOM">]>'
               '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><sheetData>'
               '<row r="1"><c r="A1" t="inlineStr"><is><t>&x;</t></is></c></row></sheetData></worksheet>')
        with self.assertRaisesRegex(xlsx_leitor.ErroXlsx, "recusado"):
            xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml.encode("utf-16")), "LISTA.SERV.NAC.")

    # Controles: formatos legítimos que não têm DOCTYPE precisam continuar sendo lidos.
    def xlsxUtf8ComBomLido(self):
        xml = b"\xef\xbb\xbf" + _planilha(_abas_b()[0][1])
        linhas = xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml), "LISTA.SERV.NAC.")
        self.assertEqual(linhas[2][1]["A"], "10101")

    def xlsxUtf16SemDoctypeLido(self):
        xml = _planilha(_abas_b()[0][1]).decode("utf-8").replace('encoding="UTF-8"', 'encoding="UTF-16"')
        linhas = xlsx_leitor.ler_aba(_xlsx(_abas_b(), sheet1=xml.encode("utf-16")), "LISTA.SERV.NAC.")
        self.assertEqual(linhas[2][1]["A"], "10101")

    def xlsxComArquivosDemaisRecusado(self):
        with mock.patch.object(xlsx_leitor, "MAX_ARQUIVOS_ZIP", 10):
            with self.assertRaisesRegex(xlsx_leitor.ErroXlsx, "limite de arquivos no ZIP"):
                xlsx_leitor.ler_aba(_xlsx(_abas_b(), extras=20), "LISTA.SERV.NAC.")

    def conversorRecusaXmlComDoctypeRc1(self):
        # A planilha é válida e tem o cabeçalho certo: só o DOCTYPE faz o conversor falhar.
        xml = _planilha(_abas_a()[0][1]).replace(b"<worksheet", b"<!DOCTYPE worksheet><worksheet", 1)
        with tempfile.TemporaryDirectory() as diretorio:
            caminho_fontes, saida = _cenario(diretorio, bytes_a=_xlsx(_abas_a(), sheet1=xml))
            self.assertEqual(_rodar(diretorio, caminho_fontes, saida, ESPERADO_FABRICADO), 1)
            self.assertTrue(_sem_saida(saida))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("uso: test_tabelas.py <nome-do-teste>")
    nome = sys.argv.pop(1)
    suite = unittest.TestSuite([TestesTabelas(nome)])
    resultado = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(0 if resultado.wasSuccessful() else 1)
