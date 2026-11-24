#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Portão sql_seguro (NL-3.1a, ordem v1 seção 1.5 e emenda v1b seção 3): executor SQL só com parâmetros vinculados.

Uso: python3 -I sql_seguro.py [RAIZ]
  RAIZ: árvore a verificar (padrão: raiz do repositório, dois níveis acima deste arquivo).

Varre src/ e apps/: arquivos com extensão de código (.h .hpp .cpp .cc .cxx .qml .cmake) e CMakeLists.txt.
Imprime sempre uma linha, mesmo com zero arquivos:
  sql_seguro: arquivos=<n> s1=<n> s2_ok=<k>/6 s3_ok=<0|1> s4_ok=<0|1> s5_ok=<0|1> s6_ok=<0|1> s7=<n> s8=<n> resultado=<PASSA|REPROVA>
Sai com 1 se REPROVA. Zero arquivo é REPROVA (piso, L-36), e erro de leitura também.

Regras (texto sensível a maiúsculas; o motor é src/foundation/sql):
  S1  texto proibido em toda a varredura (contagem); zero.
  S2  seis opções de sessão, cada uma exatamente uma vez e só em opcoes_sessao.cpp (k = quantas passam).
  S3  MYSQL_OPT_SSL_VERIFY_SERVER_CERT: pelo menos 1 em modo_aberto.cpp e 1 em modo_fixado.cpp; zero fora dos dois.
  S4  MYSQL_OPT_SSL_ENFORCE exatamente 1 vez, e em modo_aberto.cpp (o portão não lê o valor; quem prova o 0 é o
      teste tst_sql_opcoes_tls, que lê a opção de volta sem servidor, NL-3.1k).
  S5  impressão do certificado (MARIADB_OPT_TLS_PEER_FP, MARIADB_OPT_SSL_FP): pelo menos 1 em modo_fixado.cpp; zero fora.
  S6  mysql_real_connect exatamente 1 vez, em conexao.cpp.
  S7  include do conector (nome do arquivo inteiro) só em src/foundation/sql/*.cpp e src/foundation/sql/privado/;
      conta as linhas fora desses lugares.
  S8  MARIADB_TLS_DISABLE_PEER_VERIFICATION: zero em toda a varredura.
"""
import os
import re
import sys

EXTENSOES = frozenset({".h", ".hpp", ".cpp", ".cc", ".cxx", ".qml", ".cmake"})
NOMES_EXTRAS = frozenset({"CMakeLists.txt"})
RAIZES_VARRIDAS = ("src", "apps")

MOTOR = "src/foundation/sql"
ARQ_OPCOES = MOTOR + "/opcoes_sessao.cpp"
ARQ_ABERTO = MOTOR + "/modo_aberto.cpp"
ARQ_FIXADO = MOTOR + "/modo_fixado.cpp"
ARQ_CONEXAO = MOTOR + "/conexao.cpp"

S1_TOKENS = (
    "MARIADB_OPT_MULTI_STATEMENTS", "CLIENT_MULTI_STATEMENTS", "mysql_real_query", "mysql_query",
    "mysql_send_query", "mysql_real_escape_string", "mysql_escape_string", "MYSQL_READ_DEFAULT_FILE",
    "MYSQL_READ_DEFAULT_GROUP", "mysql_ssl_set", "MYSQL_OPT_SSL_KEY", "MYSQL_OPT_SSL_CERT", "MYSQL_OPT_SSL_CA",
    "MYSQL_OPT_SSL_CAPATH", "MYSQL_OPT_SSL_CIPHER", "MYSQL_OPT_SSL_CRL", "MYSQL_OPT_SSL_CRLPATH",
    "MARIADB_OPT_TLS_PEER_FP_LIST", "MARIADB_OPT_SSL_FP_LIST", "QMYSQL", "QSqlDatabase", "QSqlQuery", "QtSql",
    "Qt6::Sql", "mysql_change_user",
)
S2_TOKENS = (
    "MYSQL_OPT_LOCAL_INFILE", "MYSQL_OPT_RECONNECT", "MYSQL_INIT_COMMAND", "MARIADB_OPT_RESTRICTED_AUTH",
    "MYSQL_OPT_PROTOCOL", "MYSQL_SET_CHARSET_NAME",
)
S3_TOKEN = "MYSQL_OPT_SSL_VERIFY_SERVER_CERT"
S4_TOKEN = "MYSQL_OPT_SSL_ENFORCE"
S5_TOKENS = ("MARIADB_OPT_TLS_PEER_FP", "MARIADB_OPT_SSL_FP")
S6_TOKEN = "mysql_real_connect"
S8_TOKEN = "MARIADB_TLS_DISABLE_PEER_VERIFICATION"

# S7: include do conector pelo nome do arquivo inteiro (nunca substring solta como `ma_` dentro de `forma_`).
INCLUDE_CONECTOR = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]'
    r'(mysql\.h|mysql/[^>"]*|mariadb/[^>"]*|mariadb_com\.h|mariadb_stmt\.h|errmsg\.h|ma_[A-Za-z0-9_]+\.h)'
    r'[>"]',
    re.MULTILINE,
)


def extensao_de_codigo(nome):
    return os.path.splitext(nome)[1].lower() in EXTENSOES or nome in NOMES_EXTRAS


def ler_arvore(raiz, erros):
    """Texto de cada arquivo de src/ e apps/, pelo caminho relativo à raiz. Erro de leitura vai para `erros`."""
    def registrar(erro):
        erros.append("ERRO VARREDURA INCOMPLETA: {}: {}".format(
            getattr(erro, "filename", None) or "?", getattr(erro, "strerror", None) or str(erro)))

    textos = {}
    for base in RAIZES_VARRIDAS:
        for dirpath, _dirnames, nomes in os.walk(os.path.join(raiz, base), onerror=registrar):
            for nome in nomes:
                if not extensao_de_codigo(nome):
                    continue
                completo = os.path.join(dirpath, nome)
                rel = os.path.relpath(completo, raiz).replace(os.sep, "/")
                try:
                    with open(completo, "rb") as arquivo:
                        textos[rel] = arquivo.read().decode("utf-8", "replace")
                except OSError as erro:
                    erros.append("ERRO LEITURA {}: {}".format(rel, erro))
    return textos


def contar(textos, token):
    """Ocorrências do token em toda a varredura."""
    return sum(texto.count(token) for texto in textos.values())


def contar_em(textos, rel, token):
    """Ocorrências do token num arquivo (0 se o arquivo não existe)."""
    return textos.get(rel, "").count(token)


def regra_s1(textos):
    return sum(contar(textos, token) for token in S1_TOKENS)


def regra_s2(textos):
    """Quantas das seis opções aparecem exatamente uma vez, em opcoes_sessao.cpp, e em nenhum outro lugar."""
    return sum(
        1 for token in S2_TOKENS
        if contar_em(textos, ARQ_OPCOES, token) == 1 and contar(textos, token) == 1
    )


def regra_s3(textos):
    no_aberto = contar_em(textos, ARQ_ABERTO, S3_TOKEN)
    no_fixado = contar_em(textos, ARQ_FIXADO, S3_TOKEN)
    ok = no_aberto >= 1 and no_fixado >= 1 and contar(textos, S3_TOKEN) == no_aberto + no_fixado
    return int(ok)


def regra_s4(textos):
    ok = contar_em(textos, ARQ_ABERTO, S4_TOKEN) == 1 and contar(textos, S4_TOKEN) == 1
    return int(ok)


def regra_s5(textos):
    no_fixado = sum(contar_em(textos, ARQ_FIXADO, token) for token in S5_TOKENS)
    no_total = sum(contar(textos, token) for token in S5_TOKENS)
    return int(no_fixado >= 1 and no_total == no_fixado)


def regra_s6(textos):
    ok = contar_em(textos, ARQ_CONEXAO, S6_TOKEN) == 1 and contar(textos, S6_TOKEN) == 1
    return int(ok)


def include_permitido(rel):
    """S7: o include do conector é permitido só em src/foundation/sql/*.cpp e em src/foundation/sql/privado/."""
    if rel.startswith(MOTOR + "/privado/"):
        return True
    return os.path.dirname(rel) == MOTOR and rel.endswith(".cpp")


def regra_s7(textos):
    fora = 0
    for rel, texto in textos.items():
        if include_permitido(rel):
            continue
        fora += len(INCLUDE_CONECTOR.findall(texto))
    return fora


def regra_s8(textos):
    return contar(textos, S8_TOKEN)


def resultado(arquivos, s1, k, s3, s4, s5, s6, s7, s8, erros):
    ok = (arquivos > 0 and not erros and s1 == 0 and k == len(S2_TOKENS)
          and s3 == s4 == s5 == s6 == 1 and s7 == 0 and s8 == 0)
    return "PASSA" if ok else "REPROVA"


def main(argv):
    if len(argv) > 2:
        print("uso: sql_seguro.py [RAIZ]", file=sys.stderr)
        return 2
    raiz = os.path.abspath(argv[1]) if len(argv) == 2 else os.path.dirname(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))))
    erros = []
    textos = ler_arvore(raiz, erros)
    for linha in erros:
        print(linha)

    arquivos = len(textos)
    s1 = regra_s1(textos)
    k = regra_s2(textos)
    s3 = regra_s3(textos)
    s4 = regra_s4(textos)
    s5 = regra_s5(textos)
    s6 = regra_s6(textos)
    s7 = regra_s7(textos)
    s8 = regra_s8(textos)
    veredito = resultado(arquivos, s1, k, s3, s4, s5, s6, s7, s8, erros)
    print("sql_seguro: arquivos={} s1={} s2_ok={}/{} s3_ok={} s4_ok={} s5_ok={} s6_ok={} s7={} s8={} resultado={}".format(
        arquivos, s1, k, len(S2_TOKENS), s3, s4, s5, s6, s7, s8, veredito))
    return 0 if veredito == "PASSA" else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
