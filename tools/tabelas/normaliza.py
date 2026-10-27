# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Regras puras de normalização das tabelas fiscais (NL-1.10). Sem I/O: cada função recebe texto e devolve texto.
# Texto, nunca número: o XLSX perde o zero à esquerda do cTribNac, e o código de município e o NBS têm formatos próprios.

# Os 27 códigos de UF do IBGE, fechados (L-17: enumerados inteiros). Cada um: sigla e nome oficial.
UF_POR_CODIGO = {
    "11": ("RO", "Rondônia"),
    "12": ("AC", "Acre"),
    "13": ("AM", "Amazonas"),
    "14": ("RR", "Roraima"),
    "15": ("PA", "Pará"),
    "16": ("AP", "Amapá"),
    "17": ("TO", "Tocantins"),
    "21": ("MA", "Maranhão"),
    "22": ("PI", "Piauí"),
    "23": ("CE", "Ceará"),
    "24": ("RN", "Rio Grande do Norte"),
    "25": ("PB", "Paraíba"),
    "26": ("PE", "Pernambuco"),
    "27": ("AL", "Alagoas"),
    "28": ("SE", "Sergipe"),
    "29": ("BA", "Bahia"),
    "31": ("MG", "Minas Gerais"),
    "32": ("ES", "Espírito Santo"),
    "33": ("RJ", "Rio de Janeiro"),
    "35": ("SP", "São Paulo"),
    "41": ("PR", "Paraná"),
    "42": ("SC", "Santa Catarina"),
    "43": ("RS", "Rio Grande do Sul"),
    "50": ("MS", "Mato Grosso do Sul"),
    "51": ("MT", "Mato Grosso"),
    "52": ("GO", "Goiás"),
    "53": ("DF", "Distrito Federal"),
}


def _somente_digitos_ascii(texto):
    return texto.isascii() and texto.isdigit()


def ctribnac_texto(codigo):
    """Código de 5 ou 6 dígitos da planilha vira o texto de 6 dígitos com zero à esquerda (10101 -> 010101)."""
    texto = str(codigo).strip()
    if not _somente_digitos_ascii(texto) or len(texto) not in (5, 6):
        raise ValueError(f"cTribNac fora do formato de 5 ou 6 dígitos: {codigo!r}")
    return texto.zfill(6)


def ctribnac_confere(texto, item, subitem, desdobro):
    """O código de 6 dígitos tem de ser item, subitem e desdobro com 2 dígitos cada (sem exceção)."""
    esperado = f"{item:02d}{subitem:02d}{desdobro:02d}"
    return texto == esperado and _somente_digitos_ascii(texto) and len(texto) == 6


def codigo_municipio(texto):
    """Código IBGE de município: 7 dígitos, texto."""
    if not isinstance(texto, str) or not _somente_digitos_ascii(texto) or len(texto) != 7:
        raise ValueError(f"código de município fora do formato de 7 dígitos: {texto!r}")
    return texto


def codigo_localidade(texto):
    """Código de localidade IBGE: 7 dígitos, texto. Aceita 0000000 (águas marítimas), que não é município."""
    if not isinstance(texto, str) or not _somente_digitos_ascii(texto) or len(texto) != 7:
        raise ValueError(f"código de localidade fora do formato de 7 dígitos: {texto!r}")
    return texto


def nbs_texto(valor):
    """Código NBS preservado como está (pontuado ou de 5 dígitos), sem conversão numérica."""
    texto = str(valor).strip() if valor is not None else ""
    if not texto:
        raise ValueError("código NBS vazio")
    return texto


def grupo_sem_issqn(texto):
    """cTribNac 99XXXX: serviços sem ISSQN/ICMS (identificáveis pelo item 99)."""
    return ctribnac_texto(texto).startswith("99")


def uf_do_municipio(codigo):
    """Sigla e nome da UF a partir dos dois primeiros dígitos do código IBGE (fecha em 27)."""
    try:
        return UF_POR_CODIGO[codigo[:2]]
    except KeyError as erro:
        raise ValueError(f"código de UF desconhecido no município {codigo!r}") from erro
