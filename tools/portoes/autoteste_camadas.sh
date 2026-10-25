#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Autoteste do portão de camadas (NL-0.3, matriz 1.1, L-33 e L-36).
#
# Cada controle roda o portão numa cópia (ou na árvore real) e exige rc EXATO e a linha
# `encontrados=N analisados=M violacoes=K`, além de uma regex sobre a saída.
# Grupos: a-d (base, Front->Back, varredura vazia); e-g (include relativo, resolvido por -I
# ou DESCONHECIDO); h-j (extensões em maiúscula e .ipp/.tpp); k-l (poda só na raiz);
# B/M/F (denylist de terceiros por camada, coluna Proibido da matriz 1.1, forma de módulo e de
# classe); N (negativos: o que é permitido não reprova).
#
# Uso: autoteste_camadas.sh [RAIZ_REAL]
#   RAIZ_REAL: árvore a verificar (padrão: raiz do repositório, dois níveis acima deste script).
#   PORTAO (variável de ambiente): troca o portão testado. Só para sabotagem e para o vermelho
#   com portão real anterior, sempre sobre cópia fora da árvore (L-27).
# Cópias de trabalho em ${TMPDIR:-/var/tmp}, apagadas ao fim (L-53: só o próprio scratch).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RAIZ_REAL="${1:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
PORTAO="${PORTAO:-$SCRIPT_DIR/camadas.py}"

TMP="$(mktemp -d "${TMPDIR:-/var/tmp}/nl-autoteste-camadas.XXXXXX")" || exit 1
trap 'rm -rf -- "$TMP"' EXIT

APROVADOS=0
TOTAL=0

# Roda o portão numa única invocação. Saída em arquivo (sem pipe, L-45); devolve o rc.
rodar_portao()
{
    local raiz="$1" saida="$2"
    python3 -I "$PORTAO" "$raiz" > "$saida" 2>&1
    echo $?
}

# Confere um controle: rc exato, contagem presente, regex ERE casando a saída.
controle()
{
    local nome="$1" rc_esperado="$2" raiz="$3" regex="$4"
    local saida="$TMP/saida-$nome.txt" rc ok=1

    TOTAL=$((TOTAL + 1))
    rc="$(rodar_portao "$raiz" "$saida")"
    echo "controle $nome: rc=$rc (esperado $rc_esperado)"
    sed 's/^/    | /' "$saida"

    [ "$rc" = "$rc_esperado" ] || ok=0
    grep -Eq '^encontrados=[0-9]+ analisados=[0-9]+ violacoes=[0-9]+$' "$saida" || ok=0
    grep -Eq -- "$regex" "$saida" || ok=0

    if [ "$ok" = 1 ]; then
        echo "controle $nome: VERDE"
        APROVADOS=$((APROVADOS + 1))
    else
        echo "controle $nome: VERMELHO"
    fi
}

# Cópia de src/ e apps/ da árvore real dentro do scratch. A árvore real não é tocada.
copiar_arvore()
{
    local nome="$1" destino="$TMP/$1"
    mkdir -p "$destino" || { echo "ERRO: não criou $destino"; exit 1; }
    cp -a "$RAIZ_REAL/src" "$RAIZ_REAL/apps" "$destino/" || { echo "ERRO: não copiou para $destino"; exit 1; }
}

# Cópia + uma linha acrescentada ao arquivo relativo ARQ (cria o arquivo e o diretório se faltarem).
controle_inclusao()
{
    local nome="$1" arq="$2" rc_esperado="$3" linha="$4" regex="$5"
    copiar_arvore "$nome"
    mkdir -p "$(dirname "$TMP/$nome/$arq")" || { echo "ERRO: diretório de $arq"; exit 1; }
    printf '%s\n' "$linha" >> "$TMP/$nome/$arq" || { echo "ERRO: sabotagem $nome"; exit 1; }
    controle "$nome" "$rc_esperado" "$TMP/$nome" "$regex"
}

# Denylist: include de terceiro proibido na camada do arquivo. Regex de terceiro no texto da saída.
viol_back='^VIOLACAO src/back/camada\.cpp:[0-9]+: .*terceiro proibido'
viol_mid='^VIOLACAO src/mid/camada\.cpp:[0-9]+: .*terceiro proibido'
viol_cli='^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*terceiro proibido'
viol_gui='^VIOLACAO apps/gui/main\.cpp:[0-9]+: .*terceiro proibido'

echo "portao testado: $PORTAO"
echo "arvore real: $RAIZ_REAL"
echo "scratch: $TMP"

# --- Grupo base ---
controle a 0 "$RAIZ_REAL" '^encontrados=[1-9][0-9]* analisados=[0-9]+ violacoes=0$'
controle_inclusao b src/back/camada.cpp 1 '#include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
mkdir -p "$TMP/c/src" "$TMP/c/apps"
controle c 1 "$TMP/c" '^encontrados=0 analisados=0 '
controle_inclusao d apps/cli/main.cpp 1 '#include <nl/back/camada.h>' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: front:cli inclui nl/back/camada\.h'

# --- Include relativo: resolvido pelo arquivo ou pelo -I da camada; senão DESCONHECIDO ---
controle_inclusao e src/back/camada.cpp 1 '#include "../mid/include/nl/mid/camada.h"' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*proibido pela matriz 1\.1'
controle_inclusao f src/back/camada.cpp 1 '#include <../../mid/include/nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*proibido pela matriz 1\.1'
controle_inclusao g apps/cli/main.cpp 1 '#include "../../../nao_existe/x.h"' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*nao resolvido'
controle_inclusao h2 apps/cli/main.cpp 1 '#include "../../src/back/include/nl/back/camada.h"' \
    '^VIOLACAO apps/cli/main\.cpp:[0-9]+: .*proibido pela matriz 1\.1'

# --- Extensões: maiúsculas e .ipp/.tpp entram na varredura ---
controle_inclusao x1 src/back/y.H 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.H:1: '
controle_inclusao x2 src/back/y.ipp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.ipp:1: '
controle_inclusao x3 src/back/y.tpp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/y\.tpp:1: '

# --- Poda: nada é podado dentro de src/ ou apps/ por nome ---
controle_inclusao p1 src/back/build/x.cpp 1 '#include <nl/mid/camada.h>' '^VIOLACAO src/back/build/x\.cpp:1: '

# --- Poda só na raiz: worktrees e build-* na raiz do repositório não são varridos (L-55) ---
copiar_arvore p2
mkdir -p "$TMP/p2/worktrees/ramo/src/back" && printf '#include <nl/mid/camada.h>\n' > "$TMP/p2/worktrees/ramo/src/back/x.cpp"
controle p2 0 "$TMP/p2" 'violacoes=0$'
copiar_arvore p3
mkdir -p "$TMP/p3/build-x/src/back" && printf '#include <nl/mid/camada.h>\n' > "$TMP/p3/build-x/src/back/x.cpp"
controle p3 0 "$TMP/p3" 'violacoes=0$'

# --- Diretório ilegível: a varredura não passa em silêncio (INCOMPLETA, exit 1) ---
# Requer usuário sem poder de leitura irrestrito; como root o chmod não bloqueia e o controle reprova.
copiar_arvore o1
chmod 000 "$TMP/o1/src/back/include" || { echo "ERRO: chmod o1"; exit 1; }
controle o1 1 "$TMP/o1" '^ERRO VARREDURA INCOMPLETA: '
chmod 755 "$TMP/o1/src/back/include"

# --- Burlas do pré-processador (revisão 2): comentário, macro, digrafo, _next, emenda, string ---
controle_inclusao n1 src/back/camada.cpp 1 '/**/#include <QtSql/QSqlDatabase>' "$viol_back"
copiar_arvore n2
printf '#define NL_MID <nl/mid/camada.h>\n#include NL_MID\n' >> "$TMP/n2/src/back/camada.cpp"
controle n2 1 "$TMP/n2" '^VIOLACAO src/back/camada\.cpp:[0-9]+: .*nao literal'
controle_inclusao n3 src/back/camada.cpp 1 '%:include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n3b src/back/camada.cpp 1 '%:   include <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n4 src/back/camada.cpp 1 '#include_next <nl/mid/camada.h>' \
    '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'
controle_inclusao n5 src/back/camada.cpp 0 '#include <QtCore/QString> /* <QtSql/QSqlDatabase> */' 'violacoes=0$'
controle_inclusao n6 src/back/camada.cpp 1 'const char *s = "/*";
#include <QtSql/QSqlDatabase>' "$viol_back"
copiar_arvore n7
printf '#inc\\\nlude <nl/mid/camada.h>\n' >> "$TMP/n7/src/back/camada.cpp"
controle n7 1 "$TMP/n7" '^VIOLACAO src/back/camada\.cpp:[0-9]+: back inclui nl/mid/camada\.h'

# --- Denylist Back (Proibido: rede, SQL, cripto, XML, arquivo, Qt GUI) ---
controle_inclusao B01 src/back/camada.cpp 1 '#include <QtQuick/QQuickItem>' "$viol_back"
controle_inclusao B02 src/back/camada.cpp 1 '#include <QtQml/QQmlEngine>' "$viol_back"
controle_inclusao B03 src/back/camada.cpp 1 '#include <QtGui/QImage>' "$viol_back"
controle_inclusao B04 src/back/camada.cpp 1 '#include <QtWidgets/QWidget>' "$viol_back"
controle_inclusao B05 src/back/camada.cpp 1 '#include <QtSql/QSqlDatabase>' "$viol_back"
controle_inclusao B06 src/back/camada.cpp 1 '#include <QtNetwork/QTcpSocket>' "$viol_back"
controle_inclusao B07 src/back/camada.cpp 1 '#include <QtXml/QDomDocument>' "$viol_back"
controle_inclusao B08 src/back/camada.cpp 1 '#include <openssl/evp.h>' "$viol_back"
controle_inclusao B09 src/back/camada.cpp 1 '#include <sodium.h>' "$viol_back"
controle_inclusao B10 src/back/camada.cpp 1 '#include <libxml/parser.h>' "$viol_back"
controle_inclusao B11 src/back/camada.cpp 1 '#include <xmlsec/xmlsec.h>' "$viol_back"
controle_inclusao B12 src/back/camada.cpp 1 '#include <fstream>' "$viol_back"
controle_inclusao B13 src/back/camada.cpp 1 '#include <filesystem>' "$viol_back"
controle_inclusao B14 src/back/camada.cpp 1 '#include <QFile>' "$viol_back"
controle_inclusao B15 src/back/camada.cpp 1 '#include <QDir>' "$viol_back"
controle_inclusao B16 src/back/camada.cpp 1 '#include <QSqlDatabase>' "$viol_back"
controle_inclusao B17 src/back/camada.cpp 1 '#include <QNetworkAccessManager>' "$viol_back"
controle_inclusao B18 src/back/camada.cpp 1 '#include <QQuickItem>' "$viol_back"
controle_inclusao B19 src/back/camada.cpp 1 '#include <QDomDocument>' "$viol_back"

# --- Denylist Mid (Proibido: QML, widget) ---
controle_inclusao M01 src/mid/camada.cpp 1 '#include <QtQuick/QQuickItem>' "$viol_mid"
controle_inclusao M02 src/mid/camada.cpp 1 '#include <QtQml/QQmlEngine>' "$viol_mid"
controle_inclusao M03 src/mid/camada.cpp 1 '#include <QtWidgets/QWidget>' "$viol_mid"
controle_inclusao M04 src/mid/camada.cpp 1 '#include <QWidget>' "$viol_mid"

# --- Denylist Front (Proibido: SQL, cripto, HTTP, include de back/foundation exceto base) ---
controle_inclusao F01 apps/cli/main.cpp 1 '#include <QtSql/QSqlDatabase>' "$viol_cli"
controle_inclusao F02 apps/cli/main.cpp 1 '#include <QtNetwork/QTcpSocket>' "$viol_cli"
controle_inclusao F03 apps/cli/main.cpp 1 '#include <openssl/evp.h>' "$viol_cli"
controle_inclusao F04 apps/cli/main.cpp 1 '#include <sodium.h>' "$viol_cli"
controle_inclusao F05 apps/cli/main.cpp 1 '#include <libxml/parser.h>' "$viol_cli"
controle_inclusao F06 apps/cli/main.cpp 1 '#include <xmlsec/xmlsec.h>' "$viol_cli"
controle_inclusao F07 apps/gui/main.cpp 1 '#include <QNetworkAccessManager>' "$viol_gui"

# --- Negativos: o que a matriz permite não reprova ---
controle_inclusao N01 apps/gui/main.cpp 0 '#include <QtQuick/QQuickView>' 'violacoes=0$'
controle_inclusao N02 src/back/camada.cpp 0 '#include <QtCore/QString>' 'violacoes=0$'
controle_inclusao N03 src/foundation/base/camada.cpp 0 '#include <openssl/evp.h>' 'violacoes=0$'
controle_inclusao N04 apps/cli/main.cpp 0 '#include <nl/mid/composicao/fachada.h>' 'violacoes=0$'

echo "autoteste_camadas: $APROVADOS/$TOTAL controles verdes"
[ "$APROVADOS" = "$TOTAL" ] || exit 1
exit 0
