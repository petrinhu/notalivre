#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Converte os Anexos A e B da Biblioteca gov.br/nfse (NL-1.10) para o formato embutido: um JSON por tabela,
# com bloco de proveniência (URL, versão, data de publicação, data de download, SHA-256 e bytes da origem).
#
# Uso (stdlib, sem pip; L-51):
#   python3 -I tools/tabelas/converter.py --xlsx-dir DIR_COM_OS_XLSX --saida DIR_DE_SAIDA
#   python3 -I tools/tabelas/converter.py --so-anexo-a --xlsx-dir DIR_COM_O_XLSX_A --saida DIR_DE_SAIDA
#   (--so-anexo-a: converte só municipios, paises e localidades, do Anexo A; não lê nem exige o Anexo B)
#
# Regras: confere o SHA-256 de cada XLSX contra tools/tabelas/fontes.json antes de ler; confere as contagens
# fixadas (ESPERADO) e a consistência linha a linha. Qualquer divergência sai com status 1 e NADA é gravado.
import argparse
import json
import os
import sys
from pathlib import Path

# python3 -I não põe o diretório do script no sys.path: os módulos irmãos entram explicitamente.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import conversores  # noqa: E402
import fontes  # noqa: E402
import xlsx_leitor  # noqa: E402
from xml.etree import ElementTree  # noqa: E402
import zipfile  # noqa: E402

FONTES_PADRAO = Path(__file__).resolve().parent / "fontes.json"

# Contagens fixadas a partir da medição dos XLSX de 25/10/2026 (ver README). Divergência é falha.
ESPERADO = {
    "ctribnac": {"linhas": 580, "com_codigo": 338, "sem_codigo": 242,
                 "codigo_6_digitos": 220, "codigo_5_digitos": 118, "grupo_99": 1},
    "nbs": {"linhas": 1210, "pontuados": 1108, "sem_pontos": 102, "sem_descricao": 2},
    "municipios": {"linhas": 5570, "siglas_planilha": 450, "siglas_derivadas": 5120},
    "paises": {"linhas": 250},
    "localidades": {"linhas": 1, "vazias_finais": 1},
}

# (nome da tabela, anexo, aba, conversor, arquivo de saída)
TABELAS = [
    ("ctribnac", "B", conversores.ABA_SERVICO, conversores.converter_servico, "ctribnac.json"),
    ("nbs", "B", conversores.ABA_NBS, conversores.converter_nbs, "nbs.json"),
    ("municipios", "A", conversores.ABA_MUNICIPIOS, conversores.converter_municipios, "municipios.json"),
    ("paises", "A", conversores.ABA_PAISES, conversores.converter_paises, "paises.json"),
    ("localidades", "A", conversores.ABA_LOCALIDADES, conversores.converter_localidades, "localidades.json"),
]


def _argumentos(argv):
    analisador = argparse.ArgumentParser(description="Converte os Anexos A e B para o formato embutido (NL-1.10).")
    analisador.add_argument("--xlsx-dir", required=True, help="diretório com os XLSX dos Anexos A e B")
    analisador.add_argument("--saida", required=True, help="diretório de saída dos JSON")
    analisador.add_argument("--fontes", default=str(FONTES_PADRAO), help="fontes.json com as âncoras")
    analisador.add_argument("--so-anexo-a", action="store_true",
                            help="converte só as tabelas do Anexo A e não exige o Anexo B")
    return analisador.parse_args(argv)


def _divergencias(nome, contagens):
    divergencias = []
    for chave, esperado in ESPERADO[nome].items():
        obtido = contagens.get(chave, 0)
        if obtido != esperado:
            divergencias.append(f"{nome}: {chave} esperado {esperado}, obtido {obtido}")
    return divergencias


def _documento(nome, anexo, aba, documento_fontes, resultado):
    return {
        "proveniencia": {
            "tabela": nome,
            "anexo": anexo["letra"],
            "aba": aba,
            "versao_anexo": anexo["versao"],
            "data_publicacao": anexo["data_publicacao"],
            "url": anexo["url"],
            "arquivo": anexo["arquivo"],
            "bytes_origem": anexo["bytes"],
            "sha256_origem": anexo["sha256"],
            "data_download": documento_fontes["data_download"],
            "versao_formato": documento_fontes["versao_formato"],
            "gerado_por": "tools/tabelas/converter.py",
        },
        "contagens": {"linhas": resultado.contagens.get("linhas", 0),
                      "convertidos": len(resultado.registros),
                      "rejeitados": len(resultado.erros), **{
                          chave: valor for chave, valor in resultado.contagens.items() if chave != "linhas"}},
        "registros": resultado.registros,
    }


def _gravar(diretorio, arquivo, documento):
    """Gravação atômica: escreve ao lado e troca com os.replace."""
    destino = Path(diretorio) / arquivo
    temporario = destino.with_suffix(destino.suffix + ".tmp")
    with open(temporario, "w", encoding="utf-8") as saida:
        json.dump(documento, saida, ensure_ascii=False, indent=1)
        saida.write("\n")
    os.replace(temporario, destino)


def _tabelas_da_execucao(so_anexo_a):
    """Tabelas a converter: todas, ou só as do Anexo A com --so-anexo-a. A ordem de TABELAS não muda."""
    if so_anexo_a:
        return [tabela for tabela in TABELAS if tabela[1] == "A"]
    return list(TABELAS)


def _letras_de_anexo(tabelas):
    """Anexos que as tabelas pedem, na ordem A, B. Anexo sem tabela selecionada não é lido."""
    return [letra for letra in ("A", "B") if any(tabela[1] == letra for tabela in tabelas)]


def _conferir_anexos(diretorio_xlsx, documento_fontes, letras):
    """Confere SHA-256 e tamanho de cada anexo pedido. Devolve (anexos, falhas)."""
    anexos = {}
    falhas = []
    for letra in letras:
        anexo = dict(documento_fontes["anexos"][letra], letra=letra)
        conteudo, motivo = fontes.conferir_anexo(diretorio_xlsx, anexo)
        if motivo:
            falhas.append(f"anexo {letra}: {motivo}")
        else:
            anexos[letra] = (anexo, conteudo)
    return anexos, falhas


def executar(argumentos):
    documento_fontes = fontes.carregar_fontes(argumentos.fontes)
    tabelas = _tabelas_da_execucao(argumentos.so_anexo_a)
    anexos, falhas = _conferir_anexos(argumentos.xlsx_dir, documento_fontes, _letras_de_anexo(tabelas))
    if falhas:
        return _parar(falhas)

    documentos = {}
    for nome, letra, aba, conversor, arquivo in tabelas:
        anexo, conteudo = anexos[letra]
        try:
            linhas = xlsx_leitor.ler_aba(conteudo, aba)
        except (xlsx_leitor.ErroXlsx, ElementTree.ParseError, zipfile.BadZipFile, KeyError) as erro:
            falhas.append(f"anexo {letra} aba {aba!r}: XLSX recusado: {erro}")
            continue
        resultado = conversor(linhas)
        divergencias = _divergencias(nome, resultado.contagens)
        print(f"tabela={nome} anexo={letra} aba={aba!r} encontrados={resultado.contagens.get('linhas', 0)} "
              f"convertidos={len(resultado.registros)} rejeitados={len(resultado.erros)} "
              + " ".join(f"{chave}={valor}" for chave, valor in sorted(resultado.contagens.items())
                         if chave != "linhas"))
        falhas.extend(resultado.erros)
        falhas.extend(divergencias)
        documentos[arquivo] = _documento(nome, anexo, aba, documento_fontes, resultado)
    if falhas:
        return _parar(falhas)

    os.makedirs(argumentos.saida, exist_ok=True)
    for arquivo, documento in documentos.items():
        _gravar(argumentos.saida, arquivo, documento)
        print(f"gravado={Path(argumentos.saida, arquivo)} registros={len(documento['registros'])}")
    print("CONVERSAO OK: contagens batem e nenhuma linha rejeitada")
    return 0


def _parar(falhas):
    for falha in falhas:
        print(f"FALHA: {falha}", file=sys.stderr)
    print(f"PARADA: {len(falhas)} falha(s); nada foi gravado", file=sys.stderr)
    return 1


def main(argv=None):
    return executar(_argumentos(argv if argv is not None else sys.argv[1:]))


if __name__ == "__main__":
    sys.exit(main())
